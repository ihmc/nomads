#include "DisServiceClient.h"
#include <iostream>

using namespace std;

namespace IHMC_ACI {
DisServiceClient* DisServiceClient::_instance = 0;

DisServiceClient* DisServiceClient::getInstance() {
	if (!_instance) {
		_instance = new DisServiceClient("localhost", DEFAULT_PORT);
	}
	return _instance;
}

vector<string>* DisServiceClient::getPrioritizedClientIds(string srcNodeId, string destNodeId)
{
    MethodCallRequest request;
    request.methodName = string("getPrioritizedClientIds");
    request.numArgs = 2;
    string args[] = {srcNodeId, destNodeId};
    request.args = args;
    request.sequenceId = generateSequenceId();
    vector<string> *result = (vector<string>*) satisfyRequest(&request);
    return result;
}

vector<string>* DisServiceClient::getPrioritizedDataTags (string srcNodeId, string destNodeId, string replicationMode) {
    MethodCallRequest request;
    string args[] = {srcNodeId, destNodeId, replicationMode};
    request.args = args;
    request.methodName = string("getPrioritizedDataTags");
    request.numArgs = 3;
    request.sequenceId = generateSequenceId();
    vector<string> *result = (vector<string>*) satisfyRequest(&request);
    return result;
}

}