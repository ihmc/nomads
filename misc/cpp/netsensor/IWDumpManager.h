#ifndef NL_DUMP_MANAGER
#define NE_DUMP_MANAGER

#include "FTypes.h"
#include "IWStationDump.h"

#include <string>
#include <list>

#ifdef IW

#include <thread>
#include <mutex>
#include <atomic>


struct nl_msg;
struct nl_sock;
struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
};
namespace IHMC_NETSENSOR
{
    enum plink_state {
        LISTEN,
        OPN_SNT,
        OPN_RCVD,
        CNF_RCVD,
        ESTAB,
        HOLDING,
        BLOCKED
    };
    
    class IWParsingUtils 
    {
    public:
        static void mac_addr_n2a (char *mac_addr, unsigned char *arg);
        static int mac_addr_a2n (unsigned char *mac_addr, char *arg);
        static void print_power_mode (struct nlattr *a, char *buf);
        static void parse_tx_bitrate (struct nlattr *bitrate_attr, char *buf, int buflen);
    };

    class IWCallBacks
    {
    public:
        static int stationDumpCallback (struct nl_msg *pMsg, void *arg);
        static int debug (struct nl_msg *pMsg, void *arg);
        static int finishHandler (struct nl_msg *msg, void *arg);
        //static int ackHandler (struct nl_msg *msg, void *arg);
    };

    class IWDumpList
    {
    public:
        IWDumpList();
        ~IWDumpList();
        void put (IWStationDump& iwDump);
        void printList (void);
        IWStationDump * get (int index);
        int size (void);
        std::list<IWStationDump> * getCopy (void);
        void clear (void);

    private:
        
        std::list<IWStationDump> _IWStationList;
        std::mutex _m;
    };

	class IWDumpManager
	{
	public:
        //IWDumpManager (const char* pszIname, uint64 ui64SleepInteval = 5000);
        IWDumpManager (void);
        ~IWDumpManager (void);
        void requestTermination (void);
        void printDumps (void);
        int init (const char* pszIname, uint64 ui64SleepInteval = 5000);
        void start (void);
        const char * getIname (void);
        std::list<IWStationDump> * getDumps (void);
    private:
        std::thread *_pT = nullptr;
        std::atomic_bool _bTerminationRequested;
        IWDumpList _iwDumps;
        std::string _sIname;
        uint64 _ui64SleepInterval;
	};
}

#else //not dfined IW
namespace IHMC_NETSENSOR
{
    class IWDumpManager
    {
    public:
        //IWDumpManager (const char* pszIname, uint64 ui64SleepInteval = 5000);
        IWDumpManager(void);
        ~IWDumpManager(void);
        void requestTermination(void);
        void printDumps(void);
        int init(const char* pszIname, uint64 ui64SleepInteval = 5000);
        void start(void);
        const char * getIname(void);
        std::list<IWStationDump> * getDumps(void);
    };


    inline IWDumpManager::IWDumpManager() {}
    inline IWDumpManager::~IWDumpManager() {}
    inline void IWDumpManager::requestTermination(void) {}
    inline void IWDumpManager::printDumps(void) {}
    inline int IWDumpManager::init(const char* pszIname, uint64 ui64SleepInteval) { return -1; }
    inline void IWDumpManager::start(void) {}
    inline const char * IWDumpManager::getIname(void) { return nullptr; }
    inline std::list<IWStationDump> * IWDumpManager::getDumps(void) { return nullptr; }
}
//ifdefIW
#endif
//ifndef NL_DUMP_MANAGER
#endif