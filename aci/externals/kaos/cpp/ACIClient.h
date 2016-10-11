#ifndef KAOS_ACI_CLIENT_H
#define	KAOS_ACI_CLIENT_H

#include <string>
#include <vector>
#include <map>

#include "PolicyUpdateListener.h"
#include "Mutex.h"
#include "ConditionVariable.h"
#include "BufferedReader.h"
#include "BufferedWriter.h"
#include "TCPSocket.h"

#define RECONNECT_DELAY 5000
#define DEFAULT_PORT 3545 // FLIK on a telephone keypad

using namespace std;

namespace IHMC_ACI {
class ReceiverThread;

struct MethodCallResult {
    string sequenceId;
    string exceptionStr;
    void *result;
};

struct MethodCallRequest {
    string sequenceId;
    string methodName;
    string *args;
    int numArgs;
};

void printMap (map<string,string>*);

class ACIClient
{
    friend class ReceiverThread;
    
    protected:    
        ACIClient (string, unsigned short);
        map<string, MethodCallResult> _resultTable;
        vector<PolicyUpdateListener*> _listeners;
	NOMADSUtil::TCPSocket *_socket;
        string _host;
        unsigned short _port;
        NOMADSUtil::Mutex _resultTableMutex;
        NOMADSUtil::ConditionVariable *_resultTableCV;
        void connectViaSocket();
		static int _sequenceId;
        void* satisfyRequest (MethodCallRequest*);
		int parseMapOfStrings (string s, map<string,string> *m);
        int parseStringElement (string s, string result[]);
		int parseVectorOfStrings (string s, vector<string> *v);
		void* parseResult (string result);
		MethodCallResult parseMethodCallResult (string s);

    public:
//        const string WARNING_MESSAGE = "warning";
//        const string OK_MESSAGE = "ok";
        void addPolicyUpdateListener (PolicyUpdateListener*);
        void removePolicyUpdateListener (PolicyUpdateListener*);
        string generateSequenceId();
        string getStringRepresentation (string s);
        string getStringRepresentation (vector<string> v);
        string getStringRepresentation (int i);
};
        
}

#endif
