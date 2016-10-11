#ifndef KAOS_POLICY_UPDATE_LISTENER_H
#define	KAOS_POLICY_UPDATE_LISTENER_H

#include <vector>
#include <string>

using namespace std;

namespace IHMC_ACI {
class PolicyUpdateListener
{
    public:
        virtual void policiesWereUpdated (vector<string> affectedSubjectIds) = 0;
};
}

#endif
