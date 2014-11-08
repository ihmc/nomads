#include "ManagedServerMocket.h"

ManagedServerMocket::ManagedServerMocket (void)
{
    realSrvMocket = new ServerMocket();
}

ManagedServerMocket::!ManagedServerMocket (void)
{
    delete realSrvMocket;
    realSrvMocket = 0;
}

ManagedServerMocket::~ManagedServerMocket()
{
    ManagedServerMocket::!ManagedServerMocket();
}

// Initialize the server mocket to accept incoming connections
// Specifying a 0 for the port causes a random port to be allocated
// Returns the port number that was assigned, or a negative value in case of error
int ManagedServerMocket::listen (uint16 ui16Port)
{
    if (realSrvMocket == 0) {
        throw gcnew System::ObjectDisposedException("ManagedServerMocket");
    }

    return realSrvMocket->listen (ui16Port);
}

// Initialize the server mocket to accept incoming connections at a certain address
// Returns the port number that was assigned, or a negative value in case of error
int ManagedServerMocket::listen (uint16 ui16Port, System::String ^pszListenAddr)
{
    int retval;
    char* s = ConvertCString (pszListenAddr);
    retval = realSrvMocket->listen (ui16Port, s);
    FreeCString (s);
    return retval;
}

// Version of listen() using managed IPAddress type
int ManagedServerMocket::listen (uint16 ui16Port, System::Net::IPAddress ^pListenAddr)
{
    return listen(ui16Port, pListenAddr->ToString());
}

// Version of listen() using managed IPEndPoint type
int ManagedServerMocket::listen (System::Net::IPEndPoint ^pListenEndPoint)
{
    return listen ((uint16)pListenEndPoint->Port, pListenEndPoint->Address->ToString());
}

ManagedMocket^ ManagedServerMocket::accept (void)
{
    if (realSrvMocket == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageServerMocket");
    }

    Mocket *mock = realSrvMocket->accept();
    return gcnew ManagedMocket (mock);
}

int ManagedServerMocket::close (void)
{
    if (realSrvMocket == 0) {
        throw gcnew System::ObjectDisposedException ("ManagedMessageServerMocket");
    }
    return realSrvMocket->close();
}
