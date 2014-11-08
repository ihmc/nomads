#ifndef _MANAGED_MSG_SENDER_H
#define _MANAGED_MSG_SENDER_H

#include "FTypes.h"
#include "MessageSender.h"

namespace us {
    namespace ihmc {
        namespace mockets {
            public ref class ManagedMessageSender
            {
                public:
                    ref class Params
                    {
                        public:
                            Params (uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);
                            !Params();
                            virtual ~Params();
                            uint16 getTag (void);
                            uint8 getPriority (void);
                            uint32 getEnqueueTimeout (void);
                            uint32 getRetryTimeout (void);

                            ::MessageSender::Params *getRealParams();
                        private:
                            ::MessageSender::Params *_pRealParams;

                    }; //ref class Params

                    ManagedMessageSender (Mocket *pMock, bool bReliable, bool bSequenced);
                    !ManagedMessageSender();
                    virtual ~ManagedMessageSender();

                    // Send (enqueue for transmission) data to the remote endpoint
                    // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
                    // A tag value of 0 and a priority of 0 are used for the data, along with the default values for
                    //     the enqueue and retry timeouts
                    // Returns 0 if successful or a negative value in case of error
                    int send (array<System::Byte> ^pBuf, uint32 ui32BufSize);

                    // Send (enqueue for transmission) data to the remote endpoint
                    // The data is tagged with the specified tag value and sent using the specified priority
                    // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
                    // The default values for the enqueue and retry timeouts are used
                    // Returns 0 if successful or a negative value in case of error
                    int send (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority);

                    // Send (enqueue for transmission) data to the remote endpoint
                    // The tag, priority, enqueue timeout, and retry timeout values are specified via the Params object
                    // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
                    // Returns 0 if successful or a negative value in case of error
                    int send (array<System::Byte> ^pBuf, uint32 ui32BufSize, Params ^pParams);

                    // Send (enqueue for transmission) data to the remote endpoint
                    // The data is tagged with the specified tag value and sent using the specified priority, using the
                    //     specified enqueue and retry timeout values
                    // pBuf must point to the data to be sent and ui32BufSize must specifiy the number of bytes to send
                    // Returns 0 if successful or a negative value in case of error
                    int send (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                        uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

                    // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
                    // and then transmits the new message using the specified parameters
                    // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
                    // See documentation for cancel() and send() for more details
                    // Returns 0 if successful or a negative value in case of error
                    int replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag);

                    // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
                    // and then transmits the new message using the specified parameters
                    // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
                    // See documentation for cancel() and send() for more details
                    // Returns 0 if successful or a negative value in case of error
                    int replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, Params ^pParams);

                    // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
                    // and then transmits the new message using the specified parameters
                    // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
                    // See documentation for cancel() and send() for more details
                    // Returns 0 if successful or a negative value in case of error
                    int replace (array<System::Byte> ^pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                        uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

                    // Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
                    // Note that the messages may be pending transmission (which applies to all flows)
                    // or may have already been transmitted but not yet acknowledged (which only applies to
                    // reliable flows)
                    int cancel (uint16 ui16TagId);

                    // Set the default timeout for enqueuing data into the outgoing buffer
                    // If the outgoing buffer is full, a call to send will block until the timeout expires
                    void setDefaultEnqueueTimeout (uint32 ui32EnqueueTimeout);

                    // Set the default timeout for retransmission of reliable packets that have not been
                    // acknowledged
                    // NOTE: Setting a timeout would imply that packets may not be reliable!
                    void setDefaultRetryTimeout (uint32 ui32RetryTimeout);

                private:
                    ::MessageSender *_pRealSender;
            }; //ref class ManagedMessageSender
		} //namespace mockets
    } //namespace ihmc
}//namespace us

#endif //_MANAGED_MSG_SENDER_H