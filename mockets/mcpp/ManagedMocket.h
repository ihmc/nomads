#ifndef INCL_MANAGED_MOCKET_H
#define INCL_MANAGED_MOCKET_H

// ManagedMocket.cpp
// jk, 2/2006

#pragma unmanaged
#include "Mocket.h"
#pragma managed

#include "StringUtil.h"
#include "ManagedMessageSender.h"
#include "ManagedMocketStats.h"

using namespace us::ihmc::mockets;

namespace us {
    namespace ihmc {
        namespace mockets {

            public delegate bool PeerUnreachableWarningCallback (unsigned long ulTimeSinceLastContact);      

            public ref class ManagedMocket //: public ManagedMocket
            {
                public:
		            ManagedMocket (void);

                    // for use by ManagedMessageServerMocket
                    ManagedMocket (Mocket *realMocket);

                    !ManagedMocket (void);
                    virtual ~ManagedMocket();

                    // Register a callback function to be invoked when no data (or keepalive) has been received from the peer mocket
                    // The callback will indicate the time (in milliseconds) since last contact
                    // If the callback returns true, the mocket connection will be closed
                    // An optional argument may be passed when setting the callback which will be passed in during the callback
                    //int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);
		            int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallback^ pCallback);

                    int bind (System::String ^pszBindAddress, uint16 ui16BindPort);
                    
                    // Attempt to connect to a ServerMocket at the specified remote host on the specified remote port
                    // The host may be a hostname that can be resoved to an IP address or an IP address in string format (e.g. "127.0.0.1")
                    // The default connect timeout is 30 seconds
                    // Returns 0 if successful or a negative value in case of failure
                    int connect (System::String ^pszRemoteHost, uint16 ui16RemotePort);
                    int connect (System::Net::IPAddress ^pRemoteHost, uint16 ui16RemotePort);

                    // Same as the connect method above with the additional capability of specifying an explicit timeout value
                    // The timeout value must be in milliseconds
                    int connect (System::String ^pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout);

                    // Closes the current open connection to a remote endpoint
                    // Returns 0 if successful or a negative value in case of error
                    int close (void);

                    // Return the remote host address to which the connection has been established
                    System::Net::IPAddress^ getRemoteAddress (void);

                    // Return the remote port to which the connection has been established
                    uint16 getRemotePort();

                    // Enables or disables cross sequencing across the reliable sequenced and unreliable sequenced packets
                    int enableCrossSequencing (bool bEnable);

                    // Obtains a new sender for the specified combination of reliability and sequencing parameters
                    ManagedMessageSender ^getSender (bool bReliable, bool bSequenced);

                    // Enqueues the specified data for transmission using the specified reliability and sequencing requirements
                    // The tag identifies the type of the packet and the priority indicates the priority for the packet
                    // The enqueue timeout indicates the length of time in milliseconds for which the method will wait
                    //     if there is no room in the outgoing buffer (a zero value indicates wait forever)
                    // The retry timeout is currently not utilized, but in the future could be used to specify the length of time
                    //     for which the transmitter will retransmit the packet to ensure successful delivery
                    // Returns 0 if successful or a negative value in case of error
                    int send (bool bReliable, bool bSequenced, array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                        uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

                    // Retrieves the data from next packet that is ready to be delivered to the application
                    // At most ui32BufSize bytes are copied into the specified buffer
                    // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
                    //     whereas a timeout of -1 implies wait indefinitely
                    // NOTE: Any additional data in the packet that will not fit in the buffer is discarded
                    // Returns the number of bytes that were copied into the buffer, 0 in case of the connection
                    //     being closed, and -1 in case no data is available within the specified timeout
                    int receive (array<System::Byte> ^pBuf, uint32 ui32BufSize); // int64 i64Timeout = 0
                    int receive (array<System::Byte> ^pBuf, uint32 ui32BufSize, int64 i64Timeout);

                    // Returns the size of the next message that is ready to be delivered to the application,
                    //     0 in case of the connection being closed, and -1 in case no data is available within the specified timeout
                    // If no message is available, the call will block based on the timeout parameter
                    // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
                    //     whereas a timeout of -1 implies wait indefinitely
                    int getNextMessageSize (void);                    
                    int getNextMessageSize (int64 i64Timeout);

                    // Retrieves the data from next message that is ready to be delivered to the application
                    // A new buffer of the size necessary for the message is allocated and the pointer to the
                    //     buffer is returned.
                    // NOTE: This method is inefficient because it results in a new memory allocation for every call to receive.
                    //       Consider using getNextMessageSize and maintaining a single buffer in the application
                    // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
                    //     whereas a timeout of -1 implies wait indefinitely
                    // Returns the buffer with the message, or null in case no data is available within the specified timeout.
                    array<System::Byte>^ receive (void);
                    array<System::Byte>^ receive (int64 i64Timeout);

                    // Retrieves the data from the next message that is ready to be delivered to the application
                    // Not specifiying a timeout or a timeout of 0 implies that the default timeout should be used
                    //     whereas a timeout of -1 implies wait indefinitely
                    // The data is scattered into the buffers that are passed into the method
                    // The pointer to the buffer and the buffer size arguments must be passed in pairs
                    // The last argument should be NULL
                    // Returns the total number of bytes that were copied into all the buffers, 0 in case of the
                    //     connection being closed, and -1 in case no data is available within the specified timeout
                    // NOTE: If the caller passes in three buffers, (e.g., sreceive (-1, pBufOne, 8, pBufTwo, 1024, pBufThree, 4096)),
                    //       and the method returns 4000, the implication is that 8 bytes were read into pBufOne, 1024 bytes into
                    //       pBufTwo, and the remaining 2968 bytes into pBufThree.
                    // NOTE: Any additional data in the packet that will not fit in the buffers is discarded
                    int sreceive (int64 i64Timeout, array<System::Byte> ^pBuf1, uint32 ui32BufSize1, ... array<Object^>^ args);

                    // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
                    // and then transmits the new message using the specified parameters
                    // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
                    // See documentation for cancel() and send() for more details
                    // Returns 0 if successful or a negative value in case of error
                    int replace (bool bReliable, bool bSequenced, array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                        uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

                    // Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
                    // Note that the messages may be pending transmission (which applies to all flows)
                    // or may have already been transmitted but not yet acknowledged (which only applies to
                    // reliable flows)
                    int cancel (bool bReliable, bool bSequenced, uint16 ui16TagId);

                    // Returns a pointer to the Statistics class that maintains statistics
                    // about this mocket connection
                    // NOTE: This must not be deallocated by the caller
                    ManagedMocketStats ^getStatistics (void);

                    void setIdentifier (System::String ^pszIdentifier);
                    System::String ^getIdentifier();

                    static bool callbackUnreachablePeerWarningFn(unsigned long ulTimeSinceLastContact);
                private:
                    // holds the real thing
                    ::Mocket* _pRealMocket;
                    static PeerUnreachableWarningCallback ^_peerUnreachableWarningCallback;
            }; // ref class ManagedMocket
        } //namespace Mockets
    } //namespace IHMC
} //namespace US

#endif
