#include "net/NetworkMessageService.h"
#include "net/NetworkMessageServiceListener.h"
#include "net/NetUtils.h"

#include "Logger.h"
#include "NLFLib.h"
#include "StrClass.h"

#if defined (WIN32)
    #include <Winsock2.h>
#elif defined (UNIX)
    #include <arpa/inet.h>
#endif
#include <signal.h>

using namespace NOMADSUtil;

const String BROADCAST = "broadcast";
const String MULTICAST = "multicast";
const String REL_UNICAST = "rel_unicast";

void sigIntHandler (int sig)
{
    delete pLogger;
    pLogger = NULL;
    exit (0);
}

class TestListener : public NetworkMessageServiceListener
{
    public:
        TestListener ();
        virtual ~TestListener (void);

        virtual int messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen);
    private:
        uint64 _ui64DataMsgBytesRcvd;
        uint32 _ui32Counter;
};

void printUsage()
{
    printf ("Usage:\n");
    printf ("--send.\t\tSet the node as a sender.  NB: --send and -- listen are _not_ mutually exclusive.\n");
    printf ("--listen.\tSet the node as a listener.  NB: --send and -- listen are _not_ mutually exclusive.\n");
    printf ("-i, --interface.\tSet the network interface to be used.  All the available interfaces are used by default.\n");
    printf ("-p, --port.\tSet the port to be used.  The default port is %u\n", NetworkMessageService::DEFAULT_PORT);
    printf ("-t, --transmit.\tAllowed values are { %s | %s | %s }.  If %s is selected -r option must be specified too\n",
            (const char *)BROADCAST, (const char *)MULTICAST, (const char *)REL_UNICAST, (const char *)REL_UNICAST);
    printf ("-r, --recipient.  The IP address of the recipient node\n");
    printf ("-n, --number.\tSpecifies the number of messages to be sent.  If the node is not a sender, this value is ignored.\n");
    printf ("-s, --size.\tSpecifies the size of the message(s) to be sent.  If the node is not a sender, this value is ignored.\n");
    printf ("-d, --drop.\tSpecifies the probability a message is not sent.  If the node is not a sender, this value is ignored.\n");
    printf ("-h, --help.\tPrint this list.\n");
}

void printUsageAndExit()
{
    printUsage();
    exit(-1);
}

char * getValue (int argc, char **argv, int index)
{
    if (index <= argc) {
        return argv[index];
    }
    else {
        printUsageAndExit();
    }
}

uint32 getValueAsUI32 (int argc, char **argv, int index)
{
    return atoui32 (getValue (argc, argv, index));
}

int main (int argc, char **argv)
{
    if (signal (SIGINT, sigIntHandler) == SIG_ERR) {
        printf ("Error handling SIGINT\n");
        exit (-1);
    }

    uint32 DEFAULT_N_MSGS = 500;
    uint32 DEFAULT_MSG_SIZE = 1400;
    String DEFAULT_MODE = BROADCAST;
    uint8  DEFAULT_LOSS_PROB = 0;

    bool   isSender = false;
    bool   isListener = false;
    uint32 ui32NMsgs = DEFAULT_N_MSGS;
    uint32 ui32MsgSize = DEFAULT_MSG_SIZE;
    uint8 ui8MsgType = 4;
    String mode = DEFAULT_MODE;
    String recipient = "";
    uint8  ui8LossProb = DEFAULT_LOSS_PROB;
    String netIf = "";
    char **ppszBindingInterfaces = NULL;
    uint16 ui16Port = NetworkMessageService::DEFAULT_PORT;

    String option;
    for (int i = 1; i < argc; i++) {
        option = argv[i];
        if (option == "--send") {
            isSender = true;
        }
        else if (option == "--listen") {
            isListener = true;
        }
        else if ((option == "-t") || (option == "--transmit")) {
            String tmp = getValue (argc, argv, ++i);
            if ((tmp == BROADCAST) || (tmp == MULTICAST) || (tmp == REL_UNICAST)) {
                mode = tmp;
            }
            else {
                printUsageAndExit();
            }
        }
        else if ((option == "-r") || (option == "--recipient")) {
            recipient = getValue (argc, argv, ++i);
        }
        else if ((option == "-n") || (option == "--number")) {
            ui32NMsgs = getValueAsUI32 (argc, argv, ++i);
        }
        else if ((option == "-s") || (option == "--size")) {
            ui32MsgSize = getValueAsUI32 (argc, argv, ++i);
        }
        else if ((option == "-d") || (option == "--drop")) {
            ui8LossProb = (uint8) getValueAsUI32 (argc, argv, ++i);
        }
        else if ((option == "-i") || (option == "--interface")) {
            netIf = getValue (argc, argv, ++i);
        }
        else if ((option == "-p") || (option == "--port")) {
            ui16Port = (uint16) getValueAsUI32 (argc, argv, ++i);
        }
        else if ((option == "-h") || (option == "--help")) {
            printUsage();
            return 0;
        }
        else {
            printf ("Option %s not recognized.\n", (const char *)option);
            printUsageAndExit();
        }
    }

    // Print Configuration
    printf ("The node %s is a sender", (isSender ? "is" : "is not"));
    if (isSender) {
        printf (" and %u messages of %u bytes each are going to be sent by %s with %u probability o being dropped.\n", ui32NMsgs, ui32MsgSize, (const char *) mode, ui8LossProb);
    }
    else {
        printf (".\n");
    }
    printf ("%s instance of NetworkMessageServiceListener has been registerd.\n", (isListener ? "An" : "No"));

    pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel(Logger::L_LowDetailDebug);

    NetworkMessageService nms (((mode == BROADCAST) ? NetworkMessageService::BROADCAST : NetworkMessageService::MULTICAST));
    if (netIf == "") {
        nms.init(ui16Port);
    }
    else {
        ppszBindingInterfaces = (char**)malloc (sizeof(char*)*2);
        ppszBindingInterfaces[0] = strDup((const char*)netIf);
        ppszBindingInterfaces[1] = NULL;
        nms.init(ui16Port, (const char**)ppszBindingInterfaces);
    }
    TestListener *pListener = NULL;
    if (isListener) {
        pListener = new TestListener();
        nms.registerHandlerCallback(ui8MsgType, pListener);
    }

    srand ((uint32)getTimeInMilliseconds());

    if (isSender) {
        char * pszTestMess = (char*) malloc(ui32MsgSize);
        pszTestMess[ui32MsgSize-1] = '\0';
        uint64 ui64BytesSent = 0;
        uint32 ui32DropCounter = 0;
        for (uint32 i = 0; i < ui32NMsgs; i++) {
            if (ui8LossProb > 0) {
                float f = ((rand() % 100) + 1.0f);
                if (f < ui8LossProb) {
                    ui32DropCounter++;
                    continue;
                }
            }
            // Send the message
            if ((mode == BROADCAST) || (mode == MULTICAST)) {
                nms.broadcastMessage (ui8MsgType,       // Message Type 
                                      NULL,             // Outgoing Interfaces (If NULL all interfaces are selected)
                                      INADDR_BROADCAST, // Broadcast/Multicast Address
                                      0,                // Message Id.  If it is 0, a unique id for the message will be generated by NMS
                                      0,                // Hop Count
                                      1,                // TTL
                                      0,                // Delay Tolerance
                                      NULL,             // metadata
                                      0,                // metadata length
                                      pszTestMess,      // data
                                      ui32MsgSize,      // data length
                                      false);
            }
            else if (mode == REL_UNICAST) {
                if (recipient == "") {
                    printUsageAndExit();
                }
                in_addr *inp;
                if (inet_aton((const char*) recipient, inp) == 0) {
                    // Invalid address!
                    printf ("The recipient address %s is invalid", (const char *) recipient);
                }
                uint32 ui32TargetAddr = (uint32) inp->s_addr;
                nms.transmitReliableMessage (ui8MsgType,        // Message Type
                                             NULL,              // Outgoing Interfaces (If NULL all interfaces are selected)
                                             ui32TargetAddr,    // Unicast Address
                                             0,                 // Message Id.  If it is 0, a unique id for the message will be generated by NMS
                                             0,                 // Hop Count
                                             1,                 // TTL
                                             0,                 // Delay Tolerance
                                             NULL,              // metadata
                                             0,                 // metadata length
                                             pszTestMess,       // data
                                             ui32MsgSize);      // data length
            }
            ui64BytesSent += ui32MsgSize;
            printf ("Sent message %u/%u of %u bytes.  %llu bytes have been sent so far. %u messages have been dropped\n",
                    i, (ui32NMsgs-1), ui32MsgSize, ui64BytesSent, ui32DropCounter);
        }
        free (pszTestMess);
        pszTestMess = NULL;
    }

    // Go to sleep
    while (true) {
        sleepForMilliseconds(5000);
    }

    // End
    if (isListener) {
        delete pListener;
        pListener = NULL;
    }

    return 0;         
}

TestListener::TestListener (void)
{
    _ui64DataMsgBytesRcvd = 0;
    _ui32Counter = 0;
}

TestListener::~TestListener (void)
{
}

int TestListener::messageArrived (const char *pszIncomingInterface, uint32 ui32SourceIPAddress, uint8 ui8MsgType,
                                        uint16 ui16MsgId, uint8 ui8HopCount, uint8 ui8TTL,
                                        const void *pMsgMetaData, uint16 ui16MsgMetaDataLen,
                                        const void *pMsg, uint16 ui16MsgLen)
{
    _ui64DataMsgBytesRcvd += ui16MsgLen;
    printf ("Message %u of %u bytes received.  %u bytes received so far.\n", _ui32Counter, ui16MsgLen, _ui64DataMsgBytesRcvd);
    _ui32Counter++;
}

