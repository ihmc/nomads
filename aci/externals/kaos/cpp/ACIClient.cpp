#include <iostream>
#include <ACIClient.h>
#include <Thread.h>
#include <algorithm>

using namespace std;

namespace IHMC_ACI {

int ACIClient::_sequenceId = 0;

ACIClient::ACIClient(string host, unsigned short port)
{
    _host = host;
    _port = port;
    _resultTableCV = new NOMADSUtil::ConditionVariable(&_resultTableMutex);
    connectViaSocket();
}

/**
 * Adds a listener which will be notified when policies have been updated
 */
void ACIClient::addPolicyUpdateListener (PolicyUpdateListener *listener)
{
    _listeners.push_back(listener);
}

/**
 * Adds a listener which will be notified when policies have been updated
 */
void ACIClient::removePolicyUpdateListener (PolicyUpdateListener *listener)
{
    vector<PolicyUpdateListener*>::iterator iter = find (_listeners.begin(), _listeners.end(), listener);
    if (iter != _listeners.end()) {
        _listeners.erase(iter);
    }
}

string createMethodCallRequestStr (MethodCallRequest *request)
{
    string methodCallRequest = string("<methodCallRequest id=\"") + request->sequenceId + string("\" methodName=\"") + request->methodName + string("\"><args>");
    for (int i=0; i<request->numArgs; i++) {
        methodCallRequest += "<element type=\"string\">" + request->args[i] + "</element>";
    }
    methodCallRequest += "</args></methodCallRequest>";        
    return methodCallRequest;
}

void* ACIClient::satisfyRequest (MethodCallRequest *request)
{    
    
//    if (_writer == null)
//    {
//        throw new IOException("client is not connected");
//    }
    string requestStr = createMethodCallRequestStr (request);// + string("\n");
	int errorCode;
    if ((errorCode = _socket->sendLine(requestStr.c_str())) < 0) {
		cout << "error writing bytes, error: " << errorCode << endl;
        return NULL;
    }

    // wait for the request to be satisfied before we return
    _resultTableMutex.lock();
    map<string,MethodCallResult>::iterator it;
    while ((it = _resultTable.find(request->sequenceId)) == _resultTable.end()) {
//        try {
            _resultTableCV->wait();
//        }
//        catch (InterruptedException ie) {
//        }
    }
	_resultTableMutex.unlock();

	//cout << "got result" << endl;
    MethodCallResult result = (*it).second;
	//cout << "exception: " << result.exceptionStr << endl;
    if (result.exceptionStr.size() > 0) {
		cout << "ACIClient.satisfyRequest: remote exception: " << result.exceptionStr << endl;//" for request: " << request->methodName;
    }
    return result.result;
}

void* ACIClient::parseResult (string result)
{
//    cout << "Got result: " + result + "\n";
    int idx = result.find("<element type=\"map\"");
    if (idx == 0) {
        // TODO: check the element type and parse accordingly
        map<string,string> *m = new map<string,string>;
        parseMapOfStrings (result, m);
        return m;
    }
    idx = result.find("<element type=\"list\"");
    if (idx == 0) {
        vector<string> *v = new vector<string>;
        parseVectorOfStrings(result.substr(21), v);
        return v;
    }
}

// parses both <element type="string"> and <element type="integer">
// as string values (so we can always return maps of <string,string>)
int ACIClient::parseStringElement (string s, string result[])
{
    string startStrElement = "<element type=\"string\">";
    string endElement = "</element>";
	string startIntElement = "<element type=\"integer\">";
	string startElement = startStrElement;
	int startIdx = s.find(startStrElement);
	if (startIdx == string::npos) {
		startIdx = s.find(startIntElement);
		if (startIdx == string::npos) {
			cout << "error parsing string element: " << s << endl;
			return 0;
		}
		startElement = startIntElement;
	}
    startIdx += startElement.size();
    int endIdx = s.find(endElement, startIdx);
    result[0] = s.substr(startIdx, endIdx - startIdx);
    return endIdx + endElement.size();
}

int ACIClient::parseMapOfStrings (string s, map<string,string> *m)
{
    int currentIdx = 0;
    while (true) {
        //System.out.println("evaluating " + s.substring(currentIdx));            
        int endTagIdx = s.find("</element>", currentIdx);
        int nextIdx = s.find("<keyValuePair>", currentIdx);
        if (nextIdx == string::npos || endTagIdx < nextIdx) {
            return endTagIdx + 10;
        }

        currentIdx = nextIdx + 14;
        
        // get the key
        string key[1];
        currentIdx += parseStringElement(s.substr(currentIdx), key);        
        
        // get the value
        string value[1];
        currentIdx += parseStringElement(s.substr(currentIdx), value);
        
        // add it to the map
        (*m)[key[0]]=value[0];
    }    
}

int ACIClient::parseVectorOfStrings (string s, vector<string> *v)
{
    int currentIdx = 0;
    while (true) {
        //System.out.println("evaluating " + s.substring(currentIdx));
        int endTagIdx = s.find("</element>", currentIdx);
        int nextIdx = s.find("<element", currentIdx);
        // if we didn't find another element, or the end tag occurred before the next element, return
        if ((nextIdx == string::npos) || (endTagIdx < nextIdx)) {
            return endTagIdx + 10; //10 is length of </element>
        }

        string result[1];
        currentIdx = nextIdx + parseStringElement(s.substr(nextIdx), result);
        v->push_back(result[0]);
    }
}

void printMap (map<string,string> *myMap)
{
    typedef map<string,string> tmap;
    tmap::iterator iter = myMap->begin(), iter_end = myMap->end();
    while (iter != iter_end) {
        cout << (*iter).first << ", " << (*iter).second << "\n";
        ++iter;
        
    }
}

MethodCallResult ACIClient::parseMethodCallResult (string s)
{
    MethodCallResult result;
	result.result = NULL;
    //cout << s << "\n";
    int idxSeqStart = s.find("id=\"");
	// add length of "id=\""
	idxSeqStart += 4;
    int idxSeqEnd = s.find("\">", idxSeqStart);
    result.sequenceId = s.substr(idxSeqStart, idxSeqEnd - idxSeqStart);
    
    int idxXcpStart = s.find("<exception>");
    if (idxXcpStart != string::npos) {
		// add length of "<exception>"
		idxXcpStart += 11;
        int idxXcpEnd = s.find("</exception>");
        result.exceptionStr = s.substr(idxXcpStart, idxXcpEnd - idxXcpStart);
    }
    
    int idxResultStart = s.find("<result>");
    if (idxResultStart != string::npos) {
		// add length of "<result>"
        idxResultStart += 8;
        result.result = parseResult(s.substr(idxResultStart));
//        map<string,string> *m = new map<string,string>;
//        parseMapOfStrings (s.substr(idxResultStart), m);
//        result.result = m;
    }
    
    return result;
}

class PolicyUpdateThread : public NOMADSUtil::Thread
{
    vector<string> _subjects;
    vector<PolicyUpdateListener*> *_listeners;
    
    public:
        PolicyUpdateThread (vector<string>, vector<PolicyUpdateListener*>*);
        void run();
};

PolicyUpdateThread::PolicyUpdateThread (vector<string> subjects, vector<PolicyUpdateListener*> *listeners)
{
    _subjects = subjects;
    _listeners = listeners;
}
        
void PolicyUpdateThread::run()
{
    for (int i=0; i<_listeners->size(); i++) {
        (*_listeners)[i]->policiesWereUpdated(_subjects);		
    }                                        
}

class ReceiverThread : public NOMADSUtil::Thread
{
    ACIClient *_client;
    
    public:
        ReceiverThread (ACIClient*);
        void run();
};

ReceiverThread::ReceiverThread (ACIClient *client)
{
    _client = client;
}

void ReceiverThread::run()
{
	const int bufferSize = 4096;
    while (true) {
//            try {
        char buffer[bufferSize];
        int bytesRead;
        string s;
        //cout << "trying to read bytes";
        while ((bytesRead = _client->_socket->receiveLine(buffer, bufferSize)) > 0) {
//			cout << "got " << bytesRead << " bytes" << endl;
			// skip the last character when appending (seems to be garbage)
			s.append(buffer, bytesRead - 1);
			if (bytesRead < bufferSize) {
//				cout << "received line: " << s << endl;
				if (s.find("<methodCallResult") == 0) {
					MethodCallResult result = _client->parseMethodCallResult(s);
					_client->_resultTableMutex.lock();
					_client->_resultTable[result.sequenceId] = result;
					_client->_resultTableCV->notifyAll();
					_client->_resultTableMutex.unlock();
				}
				else if (s.find("<policyUpdate>") == 0) {
					vector<string> v;
					_client->parseVectorOfStrings(s.substr(14), &v);
					PolicyUpdateThread *t = new PolicyUpdateThread(v, &(_client->_listeners));
					t->start();					
				}
				else {
					cout << "unexpected message: " << s << endl;
				}
				s = "";
			}
        }
		cout << "exiting ReceiverThread" << endl;
    }
//            }
//            catch (Exception xcp) {
//                xcp.printStackTrace();
//                break;
//            }
}

void ACIClient::connectViaSocket()
{
//    try {
		_socket = new NOMADSUtil::TCPSocket();
                _socket->setLineTerminator("\n");
		int connectStatus = _socket->connect(_host.c_str(), _port);
		if (connectStatus < 0) {
			cout << "unable to connect to server at " << _host << ":" << _port << ", error = " << connectStatus << endl;
			return;
		}
//		_socket->sendLine("this is a test\n");
//		s->r
//        _writer = new BufferedWriter(new SocketWriter(s));
//        _reader = new BufferedReader(new SocketReader(s));
        ReceiverThread *rt = new ReceiverThread(this);
        rt->start();
//    }
//    catch (IOException xcp) {
//        throw xcp;
//    }
}

//int main()
//{    
//	string args[] = {getStringRepresentation("arg1"), getStringRepresentation(1234)};
//    MethodCallRequest request;
//    request.args = args;
//    request.methodName = string("testMethod");
//    request.numArgs = 2;
//	request.sequenceId = ACIClient::generateSequenceId();
//    cout << createMethodCallRequestStr(&request);
//    
//	ACIClient client ("localhost", DEFAULT_PORT);
//	client.satisfyRequest(&request);
//}

string ACIClient::generateSequenceId()
{
    char sequenceId[17];
    sprintf(sequenceId,"%d",_sequenceId++);
    //itoa(_sequenceId++, sequenceId, 10);
    string *s = new string(sequenceId);
    return *s;
}

string ACIClient::getStringRepresentation (string s)
{
    return "<element type=\"string\">" + s + "</element>";
}

string ACIClient::getStringRepresentation (vector<string> v)
{
    string result = "<element type=\"list\">";
    for (int i=0; i<v.size(); i++) {
        result += getStringRepresentation(v[i]);
    }
    result += "</element>";

    return result;
}

string ACIClient::getStringRepresentation (int i)
{
    char intString[17];
    sprintf(intString,"%d", i);
    //itoa(i, intString, 10);
    return string("<element type=\"integer\">") + string(intString) + string("</element>");
}
}