#ifndef KAOS_DISSERVICE_CLIENT_H
#define	KAOS_DISSERVICE_CLIENT_H

#include "ACIClient.h"
#include <vector>
#include <string>

namespace IHMC_ACI {
class DisServiceClient : public ACIClient
{
	private:
		static DisServiceClient* _instance;
		inline DisServiceClient (string host, unsigned short port) : ACIClient(host,port) {}

        public:
		vector<string>* getPrioritizedClientIds(string srcNodeId, string destNodeId);
		vector<string>* getPrioritizedDataTags (string srcNodeId, string destNodeId, string replicationMode);
		static DisServiceClient* getInstance();
};
}

#endif
