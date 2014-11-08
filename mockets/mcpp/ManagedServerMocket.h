#ifndef _MANAGED_SERVER_MOCKET_H
#define _MANAGED_SERVER_MOCKET_H

#pragma unmanaged
#include "ServerMocket.h"
#pragma managed

#include "ManagedMocket.h"

namespace us {
    namespace ihmc {
        namespace mockets {

            public ref class ManagedServerMocket
            {
                public:
                    ManagedServerMocket (void);
                    !ManagedServerMocket (void);
                    virtual ~ManagedServerMocket();

                    // Initialize the server mocket to accept incoming connections
                    // Specifying a 0 for the port causes a random port to be allocated
                    // Returns the port number that was assigned, or a negative value in case of error
                    int listen (uint16 ui16Port);

                    // Initialize the server mocket to accept incoming connections at a certain address
                    // Returns the port number that was assigned, or a negative value in case of error
                    int listen (uint16 ui16Port, System::String ^pszListenAddr);
                    int listen (uint16 ui16Port, System::Net::IPAddress ^pListenAddr);
                    int listen (System::Net::IPEndPoint ^pListenEndPoint);

                    ManagedMocket ^accept (void);

                    int close (void);

                protected:
                    ServerMocket *realSrvMocket;

            };
        }
    }
}
#endif // _MANAGED_SERVER_MOCKET_H
