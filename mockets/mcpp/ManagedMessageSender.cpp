#include "ManagedMessageSender.h"

#include "FTypes.h"
#include "MessageSender.h"
#include "ManagedMocket.h"

ManagedMessageSender::Params::Params (uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    _pRealParams = new MessageSender::Params (ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

ManagedMessageSender::Params::!Params()
{
    delete _pRealParams;
    _pRealParams = nullptr;
}

ManagedMessageSender::Params::~Params()
{
    ManagedMessageSender::Params::!Params();
}

uint16 ManagedMessageSender::Params::getTag (void)
{
    if (_pRealParams == 0) {
        throw gcnew System::ObjectDisposedException("Params");
    }
    return _pRealParams->getTag();
}

uint8 ManagedMessageSender::Params::getPriority (void)
{
    if (_pRealParams == 0) {
        throw gcnew System::ObjectDisposedException("Params");
    }
    return _pRealParams->getPriority();
}

uint32 ManagedMessageSender::Params::getEnqueueTimeout (void)
{
    if (_pRealParams == 0) {
        throw gcnew System::ObjectDisposedException("Params");
    }
    return _pRealParams->getEnqueueTimeout();
}

uint32 ManagedMessageSender::Params::getRetryTimeout (void)
{
    if (_pRealParams == 0) {
        throw gcnew System::ObjectDisposedException("Params");
    }
    return _pRealParams->getRetryTimeout();
}

::MessageSender::Params * ManagedMessageSender::Params::getRealParams()
{
    if (_pRealParams == 0) {
        throw gcnew System::ObjectDisposedException("Params");
    }
    return _pRealParams;
}

ManagedMessageSender::ManagedMessageSender (Mocket *pMock, bool bReliable, bool bSequenced)
{
    MessageSender sender = pMock->getSender (bReliable, bSequenced);
    _pRealSender = new MessageSender (sender);
}

ManagedMessageSender::!ManagedMessageSender()
{
    delete _pRealSender;
    _pRealSender = nullptr;
}

ManagedMessageSender::~ManagedMessageSender()
{
    ManagedMessageSender::!ManagedMessageSender();
}

// Send (enqueue for transmission) data to the remote endpoint
// pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
// A tag value of 0 and a priority of 0 are used for the data, along with the default values for
//     the enqueue and retry timeouts
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::send (array<System::Byte> ^pBuf, uint32 ui32BufSize)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException ("ManagedMessageSender");
    }

    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->send (buf, ui32BufSize);
}


// Send (enqueue for transmission) data to the remote endpoint
// The data is tagged with the specified tag value and sent using the specified priority
// pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
// The default values for the enqueue and retry timeouts are used
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::send (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority)
{
    if (_pRealSender == nullptr) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->send(buf, ui32BufSize, ui16Tag, ui8Priority);
}


// Send (enqueue for transmission) data to the remote endpoint
// The tag, priority, enqueue timeout, and retry timeout values are specified via the Params object
// pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::send (array<System::Byte> ^pBuf, uint32 ui32BufSize, Params ^pParams)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->send(buf, ui32BufSize, pParams->getRealParams());
}

// Send (enqueue for transmission) data to the remote endpoint
// The data is tagged with the specified tag value and sent using the specified priority, using the
//     specified enqueue and retry timeout values
// pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::send (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
          uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->send(buf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

// First cancels any previously enqueued messages that have been tagged with the specified OldTag value
// and then transmits the new message using the specified parameters
// Note that there may be no old messages to cancel - in which case this call behaves just like a send()
// See documentation for cancel() and send() for more details
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag)
{
    if (_pRealSender == nullptr) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->replace(buf, ui32BufSize, ui16OldTag, ui16NewTag);
}

// First cancels any previously enqueued messages that have been tagged with the specified OldTag value
// and then transmits the new message using the specified parameters
// Note that there may be no old messages to cancel - in which case this call behaves just like a send()
// See documentation for cancel() and send() for more details
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, Params ^pParams)
{
    if (_pRealSender == nullptr) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->replace(buf, ui32BufSize, ui16OldTag, pParams->getRealParams());
}

// First cancels any previously enqueued messages that have been tagged with the specified OldTag value
// and then transmits the new message using the specified parameters
// Note that there may be no old messages to cancel - in which case this call behaves just like a send()
// See documentation for cancel() and send() for more details
// Returns 0 if successful or a negative value in case of error
int ManagedMessageSender::replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
             uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    pin_ptr<System::Byte> buf = &pBuf[0];
    return _pRealSender->replace(buf, ui32BufSize, ui16OldTag, ui16NewTag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

// Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
// Note that the messages may be pending transmission (which applies to all flows)
// or may have already been transmitted but not yet acknowledged (which only applies to
// reliable flows)
int ManagedMessageSender::cancel (uint16 ui16TagId)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    return _pRealSender->cancel(ui16TagId);
}

// Set the default timeout for enqueuing data into the outgoing buffer
// If the outgoing buffer is full, a call to send will block until the timeout expires
void ManagedMessageSender::setDefaultEnqueueTimeout (uint32 ui32EnqueueTimeout)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    _pRealSender->setDefaultEnqueueTimeout(ui32EnqueueTimeout);
}

// Set the default timeout for retransmission of reliable packets that have not been
// acknowledged
// NOTE: Setting a timeout would imply that packets may not be reliable!
void ManagedMessageSender::setDefaultRetryTimeout (uint32 ui32RetryTimeout)
{
    if (_pRealSender == 0) {
        throw gcnew System::ObjectDisposedException("ManagedMessageSender");
    }
    _pRealSender->setDefaultRetryTimeout(ui32RetryTimeout);
}
