// ManagedMocket.cpp
// jk, 2/2006

#pragma unmanaged
#include "Mocket.h"
#pragma managed

#include "StringUtil.h"

#include "ManagedMessageSender.h"
#include "ManagedMocket.h"

bool internalPeerUnreachableWarningCallbackFunction (void *pCallbackArg, unsigned long ulTimeSinceLastContact);
bool managedInternalPeerUnreachableWarningCallbackFunction (unsigned long ulTimeSinceLastContact);

ManagedMocket::ManagedMocket (void)
{
    _pRealMocket = new ::Mocket();
}

ManagedMocket::ManagedMocket (Mocket *pRealMocket)
{
    _pRealMocket = pRealMocket;
}

ManagedMocket::!ManagedMocket (void)
{
    delete _pRealMocket;
    _pRealMocket = nullptr;
}

ManagedMocket::~ManagedMocket()
{
    ManagedMocket::!ManagedMocket();
}

// Register a callback function to be invoked when no data (or keepalive) has been received from the peer mocket
// The callback will indicate the time (in milliseconds) since last contact
// If the callback returns true, the mocket connection will be closed
// An optional argument may be passed when setting the callback which will be passed in during the callback
int ManagedMocket::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallback^ callbackFn)
{
    int retVal;

    _peerUnreachableWarningCallback = callbackFn;

    if (callbackFn == nullptr) {
        retVal = _pRealMocket->registerPeerUnreachableWarningCallback (NULL, NULL);
    }
    else {
        retVal = _pRealMocket->registerPeerUnreachableWarningCallback (&internalPeerUnreachableWarningCallbackFunction, NULL);
    }

    return 0;
}

// this function is to be registered with the Real MessageMocket as the callback function for the peerUnreachableWarning.
#pragma unmanaged
bool internalPeerUnreachableWarningCallbackFunction (void *pCallbackArg, unsigned long ulTimeSinceLastContact)
{
    return managedInternalPeerUnreachableWarningCallbackFunction(ulTimeSinceLastContact);
}
#pragma managed

inline bool managedInternalPeerUnreachableWarningCallbackFunction (unsigned long ulTimeSinceLastContact)
{
    return ManagedMocket::callbackUnreachablePeerWarningFn (ulTimeSinceLastContact);
}

bool ManagedMocket::callbackUnreachablePeerWarningFn (unsigned long ulTimeSinceLastContact)
{
    if (_peerUnreachableWarningCallback != nullptr) {
        return (*_peerUnreachableWarningCallback).Invoke (ulTimeSinceLastContact);
    }
    return false;
}

int ManagedMocket::bind (System::String ^pszBindAddress, uint16 ui16BindPort)
{
    char *pszAddr = ConvertCString (pszBindAddress);
    int retval = _pRealMocket->bind (pszAddr, ui16BindPort);
    return retval;
}

// Attempt to connect to a ServerMessageMocket at the specified remote host on the specified remote port
// The host may be a hostname that can be resoved to an IP address or an IP address in string format (e.g. "127.0.0.1")
// The default connect timeout is 30 seconds
// Returns 0 if successful or a negative value in case of failure
int ManagedMocket::connect (System::String ^pszRemoteHost, uint16 ui16RemotePort)
{
    int retval;
    char* s = ConvertCString (pszRemoteHost);
    retval = _pRealMocket->connect (s, ui16RemotePort);
    FreeCString (s);
    return retval;
}

// Connect that takes the managed IP address type
int ManagedMocket::connect (System::Net::IPAddress ^pRemoteHost, uint16 ui16RemotePort)
{
    int retval;
    System::String ^addrstr = pRemoteHost->ToString();
    char* s = ConvertCString (addrstr);
    retval = _pRealMocket->connect (s, ui16RemotePort);
    FreeCString (s);
    return retval;
}


// Same as the connect method above with the additional capability of specifying an explicit timeout value
// The timeout value must be in milliseconds
int ManagedMocket::connect (System::String ^pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout)
{
    int retval;
    char* s = ConvertCString (pszRemoteHost);
    retval = _pRealMocket->connect (s, ui16RemotePort, i64Timeout);
    FreeCString (s);
    return retval;
}

// Closes the current open connection to a remote endpoint
// Returns 0 if successful or a negative value in case of error
int ManagedMocket::close (void)
{
    return _pRealMocket->close();
}

// Enables or disables cross sequencing across the reliable sequenced and unreliable sequenced packets
int ManagedMocket::enableCrossSequencing (bool bEnable)
{
    return _pRealMocket->enableCrossSequencing (bEnable);
}

// Obtains a new sender for the specified combination of reliability and sequencing parameters
ManagedMessageSender^ ManagedMocket::getSender (bool bReliable, bool bSequenced)
{
    return gcnew ManagedMessageSender (_pRealMocket, bReliable, bSequenced);
}

// Enqueues the specified data for transmission using the specified reliability and sequencing requirements
// The tag identifies the type of the packet and the priority indicates the priority for the packet
// The enqueue timeout indicates the length of time in milliseconds for which the method will wait
//     if there is no room in the outgoing buffer (a zero value indicates wait forever)
// The retry timeout is currently not utilized, but in the future could be used to specify the length of time
//     for which the transmitter will retransmit the packet to ensure successful delivery
// Returns 0 if successful or a negative value in case of error
int ManagedMocket::send (bool bReliable, bool bSequenced, array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                                              uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealMocket->send (bReliable, bSequenced, buf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

// Retrieves the data from next packet that is ready to be delivered to the application
// At most ui32BufSize bytes are copied into the specified buffer
// Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
//     whereas a timeout of -1 implies wait indefinitely
// NOTE: Any additional data in the packet that will not fit in the buffer is discarded
// Returns the number of bytes that were copied into the buffer, 0 in case of the connection
//     being closed, and -1 in case no data is available within the specified timeout
int ManagedMocket::receive (array<System::Byte> ^pBuf, uint32 ui32BufSize )
{
    return receive (pBuf, ui32BufSize, 0);
}

int ManagedMocket::receive (array<System::Byte> ^pBuf, uint32 ui32BufSize, int64 i64Timeout)
{
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealMocket->receive (buf, ui32BufSize, i64Timeout);
}

int ManagedMocket::getNextMessageSize()
{
    return _pRealMocket->getNextMessageSize();
}

int ManagedMocket::getNextMessageSize (int64 i64Timeout)
{
    return _pRealMocket->getNextMessageSize (i64Timeout);
}

array<System::Byte>^ ManagedMocket::receive()
{
    return this->receive (0);
}

int ManagedMocket::sreceive (int64 i64Timeout, array<System::Byte> ^pBuf1, uint32 ui32BufSize1, ... array<Object^>^ args)
{
    pin_ptr<System::Byte> buf = &pBuf1[0];
    return _pRealMocket->sreceive (i64Timeout, buf, ui32BufSize1, args);
    
    return 0;
}

array<System::Byte>^ ManagedMocket::receive (int64 i64Timeout)
{
    // make the call directly into the realMsgMocket for efficiency.
    int iMsgSize = _pRealMocket->getNextMessageSize (i64Timeout);

    if (iMsgSize == -1) {
        return nullptr;
    }
    else if (iMsgSize == 0) {
        throw gcnew System::ApplicationException ("connection was closed");
    }

    array<System::Byte>^ pBuff = gcnew array<System::Byte>(iMsgSize);
    int rc = receive (pBuff, iMsgSize, i64Timeout);

    if (rc == 0) {
        throw gcnew System::ApplicationException ("connection was closed");
    }
    else if (rc == -1) {
        return nullptr;
    }
    else if (rc != iMsgSize) {
        throw gcnew System::ApplicationException ("insuficient data!");
    }
    else {
        return pBuff;
    }
}

// First cancels any previously enqueued messages that have been tagged with the specified OldTag value
// and then transmits the new message using the specified parameters
// Note that there may be no old messages to cancel - in which case this call behaves just like a send()
// See documentation for cancel() and send() for more details
// Returns 0 if successful or a negative value in case of error
int ManagedMocket::replace (bool bReliable, bool bSequenced, array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                                                 uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealMocket->replace (bReliable, bSequenced, buf, ui32BufSize, ui16OldTag, ui16NewTag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

// Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
// Note that the messages may be pending transmission (which applies to all flows)
// or may have already been transmitted but not yet acknowledged (which only applies to
// reliable flows)
int ManagedMocket::cancel (bool bReliable, bool bSequenced, uint16 ui16TagId)
{
    return _pRealMocket->cancel (bReliable, bSequenced, ui16TagId);
}

uint16 ManagedMocket::getRemotePort()
{
    return _pRealMocket->getRemotePort();
}

// Return the remote host address to which the connection has been established
System::Net::IPAddress ^ManagedMocket::getRemoteAddress (void)
{
    unsigned int rawaddr = _pRealMocket->getRemoteAddress();
    __int64 rawLongAddr = rawaddr;

    return gcnew System::Net::IPAddress (rawLongAddr);
}


// Returns a pointer to the Statistics class that maintains statistics
// about this mocket connection
ManagedMocketStats ^ManagedMocket::getStatistics (void)
{
    if (_pRealMocket == nullptr) {
        throw gcnew System::ObjectDisposedException("ManagedMocket");
    }
    return gcnew ManagedMocketStats (_pRealMocket->getStatistics());
}

void ManagedMocket::setIdentifier (System::String ^pszIdentifier)
{
    char* pszIdAux = ConvertCString (pszIdentifier);
    _pRealMocket->setIdentifier (pszIdAux);
    FreeCString (pszIdAux);
}

System::String ^ManagedMocket::getIdentifier()
{
    char *pszId = (char*)_pRealMocket->getIdentifier();
    System::String ^strAux = gcnew System::String(pszId);
    FreeCString (pszId);
    return strAux;
}
