

#include "TapInterface.h"

namespace ACMNetProxy {

    class TapInterfaceTestAndConfiguration
    {
        public:
            TapInterfaceTestAndConfiguration (void);
            ~TapInterfaceTestAndConfiguration (void);
            int runTapInterfaceTest (void);
            int updateIPAddress (int argc, char *argv[]);
            int updateMacAddress (int argc, char *argv[]);

        private:
            int checkFilePermission (const char * fileName);
            int writeTAPConfOnFile (ACMNetProxy::TapInterface *pTITest, const char * fileName);

            void showIPHelp (void);
            char * getAdapterName (char * driverDescriptor);
            bool isValidIP (char *ip);
            bool isValidMask (char *mask);
            int setIP (char * adapterName, char * newIPAddr, char * newMaskAddr, char * newGatewayAddr);
            int setIPWithoutGateway (char * adapterName, char * newIPAddr, char * newMaskAddr);

            void showMacHelp (void);
            bool isValidMAC (char *mac);
            int setMAC (char * adapterName, char * newMacAddr);
            int resetAdapter (char * adapterName);
    };
}
