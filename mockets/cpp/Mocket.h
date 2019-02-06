/*
 * Mocket.h
 *
 * This file is part of the IHMC Mockets Library/Component
 * Copyright (c) 2002-2014 IHMC.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 (GPLv3) as published by the Free Software Foundation.
 *
 * U.S. Government agencies and organizations may redistribute
 * and/or modify this program under terms equivalent to
 * "Government Purpose Rights" as defined by DFARS
 * 252.227-7014(a)(12) (February 2014).
 *
 * Alternative licenses that allow for use within commercial products may be
 * available. Contact Niranjan Suri at IHMC (nsuri@ihmc.us) for details.
 */

#ifndef INCL_MOCKET_H
#define INCL_MOCKET_H

#include "PeerStatusCallbacks.h"
#include "ACKManager.h"
#include "CancelledTSNManager.h"
#include "StateMachine.h"
#include "MocketStats.h"
#include "PacketQueue.h"
#include "StateCookie.h"

#include "ConditionVariable.h"
#include "DArray2.h"
#include "FTypes.h"
#include "Mutex.h"
#include "StrClass.h"
#include "Thread.h"
#include "ObjectDefroster.h"
#include "ObjectFreezer.h"
#ifndef MOCKETS_NO_CRYPTO
#include "security/CryptoUtils.h"
#endif

#include <stdarg.h>


namespace NOMADSUtil
{
    class UDPDatagramSocket;
    class InetAddr;
}

class CommInterface;
class MessageSender;
class MocketPolicyUpdateListener;
class MocketStatusNotifier;
class PacketProcessor;
class Receiver;
class Transmitter;


/*
 * Mocket
 *
 * The main class for a client application to use the mockets communication library
 * Similar in functionality to a socket - used by a client to establish a connection
 * to a server and then communicate with the server.
 */
class Mocket
{
    public:
        Mocket (const char *pszConfigFile = nullptr, CommInterface *pCI = nullptr, bool bDeleteCIWhenDone = false,
                bool bEnableDtls = false, const char* pathToCertificate = nullptr, const char* pathToPrivateKey = nullptr);
        ~Mocket (void);

        // Sets a string to use as the application or user friendly identifier for this mocket instance
        // The identifier is used when sending out statistics and when logging information
        // Some suggestions include the name of the application, the purpose for this mocket, etc.
        // May be set to nullptr to clear a previously set identifier
        // NOTE: The string is copied internally, so the caller does not need to preserve the string
        void setIdentifier (const char *pszIdentifier);

        // Returns the identifier for this mocket instance
        // Will return nullptr if there is no identifier set
        const char * getIdentifier (void);

        // Register a callback function to be invoked when no data (or keepalive) has been received from the peer mocket
        // The callback will indicate the time (in milliseconds) since last contact
        // If the callback returns true, the mocket connection will be closed
        // An optional argument may be passed when setting the callback which will be passed in during the callback
        int registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        // Register a callback function to be invoked when a suspend message has been received
        // The callback will indicate the time (in milliseconds) since the connection has been suspended
        // An optional argument may be passed when setting the callback which will be passed in during the callback
        int registerSuspendReceivedWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg);

        // Register a callback function to be invoked once peerUnreachable has been invoked and subsequently we have heard from the peer
        // The callback will indicate the time (in milliseconds) since last contact
        // An optional argument may be passed when setting the callback which will be passed in during the callback
        int registerPeerReachableCallback (PeerReachableCallbackFnPtr pCallbackFn, void *pCallbackArg);

        // Returns a pointer to the Statistics class that maintains statistics
        // about this mocket connection
        // NOTE: This must not be deallocated by the caller
        MocketStats * getStatistics (void);

        // Binds the local end point to a particular address (interface) and port.
        // Calls to this method will work if invoked before calling 'connect()'.
        // Otherwise, it will return an error code.
        int bind (const char *pszBindAddr, uint16 ui16BindPort);

        // Attempt to connect to a ServerMocket at the specified remote host on the specified remote port
        // The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1")
        // The default connect timeout is 30 seconds
        // Returns 0 if successful or a negative value in case of failure
        int connect (const char *pszRemoteHost, uint16 ui16RemotePort);

        // Same as the connect method above with the additional capability of specifying an explicit timeout value
        // The timeout value must be in milliseconds.
        int connect (const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout);

        // Same as the connect method above with the additional capability of specifying an explicit timeout value and
        // to specify if security keys should be exchange at connection time.
        // The timeout value must be in milliseconds.
        // If security keys are exchanged reEstablishConn (supports change in the network attachment point) is supported
        // both for client and server side as well as simpleSuspend for client side.
        // A default value can be used for the timeout parameter.
        int connect (const char *pszRemoteHost, uint16 ui16RemotePort, bool bPreExchangeKeys, int64 i64Timeout = 0);

        // Connect to the remote host after a change of the machine's IP address and/or port due to a change in the network attachment
        // Returns 0 if successful or a negative value in case of failure
        int reEstablishConn (uint32 ui32ReEstablishTimeout = DEFAULT_RESUME_TIMEOUT);

        // Initialize a new Mocket after a suspension
        // Create an objectDefroster to extract values from the previous node
        // Connect to the remote host and exchange the messages resume, resume_ack
        int resumeAndRestoreState (NOMADSUtil::Reader *pr, uint32 ui32ResumeTimeout = DEFAULT_RESUME_TIMEOUT);

        // Attempt to connect to a ServerMocket at the specified remote host on the specified remote port
        // The host may be a hostname that can be resolved to an IP address or an IP address in string format (e.g. "127.0.0.1")
        // The connection attempt is asynchronous, this call will return 0 on success and a
        // callback will notify the application when the connection attempt succeeded or failed.
        // Returns 0 if successful or a negative value in case of failure
        int connectAsync (const char *pszRemoteHost, uint16 ui16RemotePort);

        // Check whether a connection has been established for this mocket.
        // To use with connectAsync.
        // Returns 1 if the connection is established
        // Returns 0 if the connection process is in progress
        // Returns <0 (the error code returned by connect()) if the connection process failed, no connection was established
        int finishConnect (void);

        // Return the remote host address to which the connection has been established
        uint32 getRemoteAddress (void);

        // Return the remote port to which the connection has been established
        uint16 getRemotePort (void);

        // Return the address on the local host to which this connection is bound to
        uint32 getLocalAddress (void);

        // Return the port on the local host to which this connection is bound to
        uint16 getLocalPort (void) const;

        // Returns true if the mocket is currently connected
        bool isConnected (void);

        // Returns true if the mocket connection is encrypted via DTLS
        bool isEncrypted (void) const;

        // Closes the current open connection to a remote endpoint
        // Returns 0 if successful, 0 in case of the connection
        // being closed, and negative value in case of error
        int close (void);

        // Shutdowns the commInterface and enables the fast (non-blocking) delete of the mocket istance.
        // This should only be used in situations in which the application needs to free resources quickly
        // without waiting for the Mocket to go through a graceful close process.
        // Another possible use case is if the application knows that the remote Mocket endpoint is not avaiable
        // anymore, hence waiting for the connection timeout to expire would be a waste of time and resources.
        int applicationAbort (void);

        // Invoked by the application to suspend the mocket
        // Returns 0 in case of success and negative values in case of error
        int suspend (uint32 ui32FlushDataTimeout = DEFAULT_FLUSH_DATA_TIMEOUT, uint32 ui32SuspendTimeout = DEFAULT_SUSPEND_TIMEOUT);

        // Invoked by the application if the suspend ends with success
        // Create an ObjectFreezer that contains the state of the mocket connection
        int getState (NOMADSUtil::Writer *pw);

        // Enables or disables cross sequencing across the reliable sequenced and unreliable sequenced packets
        int enableCrossSequencing (bool bEnable);

        // Returns the current setting for cross sequencing
        bool isCrossSequencingEnabled (void);

        // Obtains a new sender for the specified combination of reliability and sequencing parameters
        MessageSender getSender (bool bReliable, bool bSequenced);

        // Returns the amount of space available in the outgoing (transmit) buffer, which
        // implies that any call to send() or gsend() with a message size that is less than
        // this value will not block.
        // NOTE: Large messages may be fragmented, resulting the message using up more space
        // Therefore, do not assume this is an exact value
        uint32 getOutgoingBufferSize (void);

        // Enqueues the specified data for transmission using the specified reliability and sequencing requirements
        // The tag identifies the type of the packet and the priority indicates the priority for the packet
        // The enqueue timeout indicates the length of time in milliseconds for which the method will wait
        //     if there is no room in the outgoing buffer (a zero value indicates wait forever)
        // The retry timeout indicates the length of time for which the transmitter will retransmit the packet to
        // ensure successful delivery (a zero value indicates retry with no time limit)
        // Returns 0 if successful or a negative value in case of error
        int send (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                  uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);


        // Variable argument version of send (to handle a gather write)
        // Caller can pass in any number of buffer and buffer size pairs
        // NOTE: The last argument, after all buffer and buffer size pairs, must be nullptr
        int gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                   const void *pBuf1, uint32 ui32BufSize1, ...);

        // Variable argument version of send (to handle a gather write)
        int gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                   const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2);

        // Returns the size of the next message that is ready to be delivered to the application,
        //     -1 in case of the connection being closed, and 0 in case no data is available within the specified timeout
        // If no message is available, the call will block based on the timeout parameter
        // Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        int getNextMessageSize (int64 i64Timeout = 0);

        // Returns the cumulative size of all messages that are ready to be delivered to the application
        //     0 in the case of no messages being available
        // NOTE: This method does not provide an indication that the connection has been closed
        uint32 getCumulativeSizeOfAvailableMessages (void);

        // Retrieves the data from next message that is ready to be delivered to the application
        // At most ui32BufSize bytes are copied into the specified buffer
        // Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        // NOTE: Any additional data in the packet that will not fit in the buffer is discarded
        // Returns the number of bytes that were copied into the buffer, -1 in case of the connection
        //     being closed, and 0 in case no data is available within the specified timeout
        int receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout = 0);

        // Retrieves the data from next message that is ready to be delivered to the application
        // A new buffer of the size necessary for the message is allocated and the pointer to the
        //     buffer is copied into ppBuf
        // NOTE: The application is responsible for deallocating the memory by calling free()
        // NOTE: This method is inefficient because it results in a new memory allocation for every receive
        //       Consider using getNextMessageSize and maintaining a single buffer in the application
        // Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        // Returns the size of the message (which is also the size of the allocated buffer), -1 in case of
        //     the connection being closed, and 0 in case no data is available within the specified timeout
        int receive (void **ppBuf, int64 i64Timeout = 0);

        // Retrieves the data from the next message that is ready to be delivered to the application
        // Not specifying a timeout or a timeout of 0 implies that the default timeout should be used
        //     whereas a timeout of -1 implies wait indefinitely
        // The data is scattered into the buffers that are passed into the method
        // The pointer to the buffer and the buffer size arguments must be passed in pairs
        // The last argument should be nullptr
        // Returns the total number of bytes that were copied into all the buffers, -1 in case of the
        //     connection being closed, and 0 in case no data is available within the specified timeout
        // NOTE: If the caller passes in three buffers, (e.g., sreceive (-1, pBufOne, 8, pBufTwo, 1024, pBufThree, 4096)),
        //       and the method returns 4000, the implication is that 8 bytes were read into pBufOne, 1024 bytes into
        //       pBufTwo, and the remaining 2968 bytes into pBufThree.
        // NOTE: Any additional data in the packet that will not fit in the buffers is discarded
        int sreceive (int64 i64Timeout, void *pBuf1, uint32 ui32BufSize1, ...);

        // First cancels any previously enqueued messages that have been tagged with the specified OldTag value
        // and then transmits the new message using the specified parameters
        // Note that there may be no old messages to cancel - in which case this call behaves just like a send()
        // See documentation for cancel() and send() for more details
        // Returns 0 if successful or a negative value in case of error
        int replace (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                     uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout);

        void setLocalAddr(const char *pszLocalAddr);

        // Cancels (deletes) previously enqueued messages that have been tagged with the specified tag
        // Note that the messages may be pending transmission (which applies to all flows)
        // or may have already been transmitted but not yet acknowledged (which only applies to
        // reliable flows)
        int cancel (bool bReliable, bool bSequenced, uint16 ui16TagId);

        // Sets the length of time (in milliseconds) for which a connection should linger before
        // closing in case there is unsent data
        // A timeout value of 0 implies that the connection should wait indefinitely until all data
        // has been sent
        int setConnectionLingerTime (uint32 ui32LingerTime);

        // Returns the current setting for the connection linger time
        uint32 getConnectionLingerTime (void);

        // Returns the current MTU that is in effect
        uint16 getMTU (void);

        // Returns the maximum MTU that may be used
        static uint16 getMaximumMTU (void);

        // Activates bandwidth estimation
        //Must be called after connect()
        int activateBandwidthEstimation (uint16 ui16InitialAssumedBandwidth = DEFAULT_INITIAL_ASSUMED_BANDWIDTH);

        // Activates congestion control
        int activateCongestionControl (void);

        // Used to activate debugging of the state capture during mockets migration.
        // This will disable sending of messages with odd sequence number in order
        // to perform a migration with messages in the queues.
        void debugStateCapture (void);

        // Activates two-way handshake instead of default four-way handshake
        void useTwoWayHandshake (void);

        // Activate a mechanism similar to slow-start to adjust the sending rate
        // to the channel bandwidth and the channel capacity
        //void useTransmissionRateModulation (void);

        // Set a bandwidth limit on the outgoing flow of data. ui32TransmitRateLimit
        // is specified in bytes per second. A value of 0 indicates no limit
        int setTransmitRateLimit (uint32 ui32TransmitRateLimit);

        // API methods for easy configuration of satellite links
        void setKeepAliveTimeout (uint16 ui16Timeout);  // Even when keepAlive are disabled this timeout is used to trigger peerUnreachable callbacks
        void disableKeepAlive (void);
        // InitialAssumedRTT, minimumRTT and maximumRTT are used together to calculate the retransmission timeout (RTO) of packets
        void setInitialAssumedRTT (uint32 ui32RTT);
        // The value of the maximum RTO is zero by default. A value of zero means no maximum RTO
        void setMaximumRTO (uint32 ui32RTO);
        void setMinimumRTO (uint32 ui32RTO);
        // RTO factor and RTO constant are used in the calculation of the RTO for the packet according to this formula:
        // (_fSRTT + RTOConstant) * RTOFactor * (1 + pWrapper->getRetransmitCount())
        void setRTOFactor (float fRTOFactor);
        void setRTOConstant (uint16 ui16RTOConstant);
        void disableRetransmitCountFactorInRTO (void);  // Disables the the factor (1 + pWrapper->getRetransmitCount()) from RTO calculation
        void setMaximumWindowSize (uint32 ui32WindowSize);
        void setSAckTransmitTimeout (uint16 ui16SAckTransTO);
        void setConnectTimeout (uint32 ui32ConnectTO);
        void setUDPReceiveConnectionTimeout (uint16 ui16UDPRecConTO);   // Low level socket timeout at connection time
        void setUDPReceiveTimeout (uint16 ui16UDPRecTO);    // Low level socket timeout after connection is open

        // Different configuration files are defined for different type of networks.
        // The application can call readConfigFile() and pass in the path to the configuration file
        // that should be loaded.
        int readConfigFile (const char *pszConfigFile);

        // When the behavior of a node is to be connected for some time, then disconnected for some time and
        // then connected again and so on, the application may wish to reset the transmission counters upon
        // reconnection so the communication won't suffer from the period of unreachability
        // The values reset with this function are estimated RTT and the transmit count and transmit timeout
        // of the packets waiting for transmission in the unacknowledged packet queue
        void resetTransmissionCounters (void);

        // Enable or disable the packet transmit log
        void enableTransmitLogging (bool bEnableXMitLogging);


    public:
        static const uint16 DEFAULT_STATS_PORT = 1400;
        static const uint16 DEFAULT_STATS_UPDATE_INTERVAL = 5000;
        static const uint16 DEFAULT_MTU = 1450;
        static const uint16 MAXIMUM_MTU = 2048;
        static const uint32 DEFAULT_CONNECT_TIMEOUT = 30000;
        static const uint32 DEFAULT_UDP_RECEIVE_CONNECTION_TIMEOUT = 250;
        static const uint16 DEFAULT_PRE_EXCHANGE_KEYS_CONNECT_RECEIVE_TIMEOUT = 2000;
        static const int32  DEFAULT_RECEIVE_TIMEOUT = -1;  // Wait indefinitely - not used
        static const uint16 DEFAULT_UDP_BUFFER_SIZE = 65535;
        static const uint32 DEFAULT_UDP_RECEIVE_TIMEOUT = 1200;
        static const uint32 DEFAULT_PENDING_PACKET_QUEUE_SIZE = 32768;
        static const bool   DEFAULT_CROSS_SEQUENCING_SETTING = true;
        static const uint16 DEFAULT_KEEP_ALIVE_TIMEOUT = 1000;
        static const uint32 DEFAULT_INITIAL_ASSUMED_RTT = 100;
        static const uint32 DEFAULT_MINIMUM_RTT = 50;
        static const uint32 DEFAULT_MAXIMUM_RTT = 5000;
        static const uint32 DEFAULT_MAXIMUM_RTO = 0;    // A value of zero means no maximum RTO
        static const uint32 DEFAULT_MINIMUM_RTO = 10;
        static const uint16 DEFAULT_RTO_FACTOR = 2;
        static const uint16 DEFAULT_RTO_CONSTANT = 0;
        static const uint16 DEFAULT_UNRELIABLE_SEQUENCED_DELIVERY_TIMEOUT = 3000;
        static const uint32 DEFAULT_MAXIMUM_WINDOW_SIZE = 262144;
        //static const uint32 DEFAULT_MAXIMUM_WINDOW_SIZE = 1048576;
        static const uint32 DEFAULT_COOKIE_LIFESPAN = 60000;
        static const uint32 DEFAULT_LINGER_TIME = 0;       // Linger indefinitely
        /*!!*/ // Talk to Mauro about this delayed ack mechanism
        // static const /int DELAYED_ACK_TIMEOUT = 200; // 200 milliseconds is what most TCP implementations use for delayed acks
        static const uint16 DEFAULT_SHUTDOWN_TIMEOUT = 2000;
        static const uint16 DEFAULT_SACK_TRANSMIT_TIMEOUT = 5;
        static const uint16 DEFAULT_CANCELLED_TSN_TRANSMIT_TIMEOUT = 1000;
        static const uint32 DEFAULT_TRANSMIT_RATE_LIMIT = 0;    // Rate limit in bytes per second
        static const uint16 DEFAULT_MAX_SHUTDOWN_ATTEMPTS = 5;
        static const uint32 DEFAULT_MAX_INTERVAL_BETWEEN_RTT_TIMESTAMPS = 15000;
        static const uint32 DEFAULT_SUSPEND_TIMEOUT = 5000;
        static const uint32 DEFAULT_RESUME_TIMEOUT = 5000;
        static const uint32 DEFAULT_FLUSH_DATA_TIMEOUT = 15000;
        static const uint32 DEFAULT_MIN_SUSPEND_TIMEOUT = 10;
        static const uint32 DEFAULT_MIN_RESUME_TIMEOUT = 10;
        static const uint32 DEFAULT_SEND_NEW_SUSPEND_RESUME_MSG_TIMEOUT = 1000;
        //static const uint16 DEFAULT_UUID_LENGTH = 8;
        static const uint16 DEFAULT_PASSWORD_LENGTH = 8;

        // Default values to create a new bandwidth estimator
        static const uint32 DEFAULT_BAND_EST_MAX_SAMPLES_NUMBER = 1005;
        static const uint32 DEFAULT_BAND_EST_TIME_INTERVAL = 1000; // milliseconds
        static const uint64 DEFAULT_BAND_EST_SAMPLING_TIME = 500;
        static const uint16 DEFAULT_INITIAL_ASSUMED_BANDWIDTH = 800; // KBps

        static const uint32 DEFAULT_TRANSMISSION_RATE_MODULATION_INITIAL_THRESHOLD = 5000; // Bytes per second

        // Default interval used when evaluating bytes to send for bandwidth limitation
        static const uint32 DEFAULT_BANDWIDTH_LIMITATION_DEFAULT_INTERVAL = 1000;

        // Default interval used when evaluating bytes sent (for bandwidth limitation)
        static const uint16 BANDWIDTH_LIMITATION_DEFAULT_INTERVAL = 100;

        // There are two mechanisms to limit the bandwidth (transmission rate) one is more
        // suited for small bandwidth limits (<51200 Bps) the other for larger.
        static const uint16 BANDWIDTH_LIMITATION_THRESHOLD  = 51200;

    private:
        friend class Packet;
        friend class PacketProcessor;
        friend class Receiver;
        friend class ServerMocket;
        friend class StreamMocket;
        friend class Transmitter;
        friend class TermSync;
        friend class AsynchronousConnector;
        friend class MocketPolicyUpdateListener;
        friend class CongestionController;
        friend class TransmissionRateModulation;

        Mocket(StateCookie cookie, NOMADSUtil::InetAddr *pRemoteAddr, const char *pszConfigFile, CommInterface *pCI, bool bDeleteCIWhenDone = false, bool bEnableDtls = false, const char* pathToCertificate = nullptr, const char* pathToPrivateKey = nullptr);
        void startThreads (void);
        int initParamsFromConfigFile (const char *pszConfigFile);
        StateMachine * getStateMachine (void);
        CommInterface * getCommInterface (void);
        Receiver * getReceiver (void);
        Transmitter * getTransmitter (void);
        PacketProcessor * getPacketProcessor (void);
        ACKManager * getACKManager (void);
        CancelledTSNManager * getCancelledTSNManager (void);
        MocketStatusNotifier * getMocketStatusNotifier (void);

        uint32 getOutgoingValidation (void);
        uint32 getIncomingValidation (void);
        StateCookie * getStateCookie (void);

        const char *getStatsIP (void);
        uint16 getStatsPort (void);
        uint16 getStatsUpdateInverval (void);
        int32  getReceiveTimeout (void);
        uint16 getUDPReceiveTimeout (void);
        uint16 getKeepAliveTimeout (void);
        bool usingKeepAlive (void);
        uint32 getInitialAssumedRTT (void);
        uint32 getMinimumRTT (void);
        //uint32 getMaximumRTT (void);
        // This is currently not used
        uint32 getMaximumRTO (void);
        uint32 getMinimumRTO (void);
        float  getRTOFactor (void);
        uint16 getRTOConstant (void);
        uint16 getUnreliableSequencedDeliveryTimeout (void);
        uint32 getMaximumWindowSize (void);
        uint16 getShutdownTimeout (void);
        uint16 getSAckTransmitTimeout (void);
        uint16 getCancelledTSNTransmitTimeout (void);
        uint16 getMaxShutdownAttempts (void);
        uint32 getMaxIntervalBetweenRTTTimestamps (void);
        uint32 getMaxSuspendTimeout (void);
        uint32 getMaxFlushDataTimeout (void);
        uint32 getMaxSendNewSuspendResumeTimeout (void);
        uint32 getTransmitRateLimit (void);
        uint32 getBandEstMaxSamplesNumber (void);
        uint32 getBandEstTimeInterval (void); // in milliseconds
        uint64 getBandEstSamplingTime (void);

        // Check if fast retransmit is enabled
        bool usingFastRetransmit (void);

        // Check if the bandwidth estimation receiver side is enabled
        bool usingRecBandEst (void);

        // Return the initial assumed bandwidth used to create a new bandwidth estimator object
        uint16 getInitialAssumedBandwidth (void);

        // Returns the transmission rate modulation initial threshold used to initialize the
        // bandwidth estimation and the transmit limit when using transmission rate modulation congestion control
        uint32 getTransmissionRateModulationInitialThreshold (void);

        void packetProcessorTerminating (void);
        void receiverTerminating (void);
        void transmitterTerminating (void);

        // Invoked by TermSync after the packet processor, receiver, and transmitter have all terminated
        void closed (void);

        // Invoked in case of simultaneous suspension
        // Return 0 if the local node is selected for suspension, 1 if it is not
        // Return -1 if it is not possible to resolve the conflict
        int resolveSimultaneousSuspension (void);

        // Methods to manage the UUID of the connection and keys used in case of suspend and resume and reconnect
        void newMocketUUID (void);
        uint32 getMocketUUID (void);
        void setMocketUUID (uint32 ui32MocketUUID);
        // Create the new secret key, invoked by the node that is being suspended
        void newSecretKey (void);
        // Create a new secret key from the password received, invoked by the node that called suspend
        void newSecretKey (char *pszPassword);
        void setPassword (const char *pszPassword);
        char * getPassword (void);
        uint16 getPasswordLength (void);
        void setKeysExchanged (bool bValue);
        bool areKeysExchanged (void);
        void setSupportReEstablish (bool bValue);
        bool supportReEstablish (void);

        bool isDebugStateCapture (void);

        // Resets the remote address after a successful reEstablishConn
        void resetRemoteAddress (uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort);

        void notifyTransmitter (void);
        void notifyPacketProcessor (void);

        // Method to insert in the objectFreezer the frozen mocket variables
        int freeze (NOMADSUtil::ObjectFreezer &objectFreezer);
        // Method to extract the frozen mocket variables
        int defrost (NOMADSUtil::ObjectDefroster &objectDefroster);

    private:
        NOMADSUtil::String _identifier;
        PeerUnreachableWarningCallbackFnPtr _pPeerUnreachableWarningCallbackFn;
        void *_pPeerUnreachableCallbackArg;
        PeerReachableCallbackFnPtr _pPeerReachableCallbackFn;
        void *_pPeerReachableCallbackArg;
        PeerUnreachableWarningCallbackFnPtr _pSuspendReceivedWarningCallbackFn;
        void *_pSuspendReceivedCallbackArg;
        MocketStats _stats;
        StateMachine _sm;
        bool _bEnableCrossSequencing;
        bool _bOriginator;   // true if this is the endpoint that opened the connection; false if this is the endpoint created to accept an incoming connection
        uint16 _ui16UDPBufferSize;
        uint32 _ui32RemoteAddress;
        uint16 _ui16RemotePort;
        uint32 _ui32LocalAddress;
        uint16 _ui16LocalPort;
        CommInterface *_pCommInterface;
        bool _bLocallyCreatedCI;
        bool _bDeleteCIWhenDone;
        bool _bEnableDtls;
        PacketProcessor *_pPacketProcessor;
        Receiver *_pReceiver;
        Transmitter *_pTransmitter;
        StateCookie _stateCookie;
        ACKManager _ackManager;
        CancelledTSNManager _cancelledTSNManager;
        MocketStatusNotifier *_pMocketStatusNotifier;

        // The following are parameters that control the runtime behavior of Mockets
        // These values are initialized using constants, but can be overridden using
        // the mockets.conf file
        uint16 _ui16MTU;
        uint32 _ui32ConnectTimeout;
        uint32 _ui32UDPReceiveConnectionTimeout;
        uint32 _ui32UDPReceiveTimeout;
        uint16 _ui16KeepAliveTimeout;
        bool _bUsingKeepAlive;
        uint32 _ui32InitialAssumedRTT;
        uint32 _ui32MaximumRTO;
        uint32 _ui32MinimumRTO;
        float  _fRTOFactor;
        uint16 _ui16RTOConstant;
        bool _bUseRetransmitCountInRTO;
        uint32 _ui32MaximumWindowSize;
        uint32 _ui32LingerTime;
        uint16 _ui16SAckTransmitTimeout;
        uint32 _ui32TransmitRateLimit;
        bool _bEnableXMitLogging;
        bool _bEnableRecvLogging;
        bool _bUseBandwidthEstimation;
        // Variables to create a new bandwidth estimator
        uint32 _ui32BandEstMaxSamplesNumber;
        uint32 _ui32BandEstTimeInterval;
        uint64 _ui64BandEstSamplingTime;
        uint16 _ui16InitialAssumedBandwidth;
        uint32 _ui32TransmissionRateModulationInitialThreshold; // Byte per second

        const char *_pszCongestionControl;
        const char *_pszNotificationIPAddress;
        const char *_pszLocalAddress;

        bool _bUseTwoWayHandshake;
        bool _bUsingFastRetransmit;
        bool _bUseReceiverSideBandwidthEstimation;
        bool _bIsServer;

        // These three variables are for the suspend/resume timeout
        uint32 _ui32SuspendTimeout;
        uint32 _ui32FlushDataTimeout;

        #ifdef MOCKETS_NO_CRYPTO
            // Dummy placeholder values
            void *_pKeyPair;
            void *_pSecretKey;
        #else
            // PublicKeyPair used only in suspend/resume process.
            // Pair of key generated from the side that starts the suspension
            // or used by the side that receives the suspend message to store the public key of the other side.
            // Regenerated for every new suspension.
            PublicKeyPair *_pKeyPair;

            // SecretKey used only in suspend/resume process.
            // Generated by the side that receives the suspend message
            // and used by the side that generate the suspension process to store the key of the other side.
            // Regenerated for every new suspension.
            SecretKey *_pSecretKey;
        #endif

        // Random generated password used to initialize a new secret key.
        // This password is also sent to the other node so it can reinitialize the same secret key
        char *_pszPassword;

        // Used only in suspend/resume process. This is the nonce used to check the identity of the node during the resume
        uint32 _ui32MocketUUID;

        // Mutex used in the suspend process
        NOMADSUtil::Mutex _mSuspend;

        bool _bMocketAlreadyBound;

        // Variable used to debug the state capture process
        bool _bSendOnlyEvenPackets;

        // Booleans to check if security keys have been exchanged: note that _bKeysExchanged corresponds
        // to all the keys and security features (public, private, secret keys and connection UUID) and
        // if set to true means that all migration operations are supported (suspend/resume and reEstablish)
        // while _bSupportReEstablish correspond to only the secret key and the UUID and onli reEstablishConn is supported.
        bool _bKeysExchanged;
        bool _bSupportReEstablish;

        //AsynchronousConnector *_pAsyncConnector;

        class TermSync {
            public:
                TermSync (Mocket *pMocket);
                void packetProcessorTerminating (void);
                void receiverTerminating (void);
                void transmitterTerminating (void);
                void waitForComponentTermination (void);
            private:
                void checkIfAllTerminated (void);
            private:
                NOMADSUtil::Mutex _m;
                NOMADSUtil::ConditionVariable _cv;
                Mocket *_pMocket;
                volatile bool _bPacketProcessorTerminated;
                volatile bool _bReceiverTerminated;
                volatile bool _bTransmitterTerminated;
        } _termSync;


        class AsynchronousConnector : public NOMADSUtil::Thread {
            public:
                AsynchronousConnector (Mocket *pMocket, const char *pszRemoteHost, uint16 ui16RemotePort);
                //void connect (const char *pszRemoteHost, uint16 ui16RemotePort);
                void run (void);
                bool isRunning (void);
                int connectionRes (void);
            private:
                Mocket *_pMocket;
                const char *_pszHost;
                uint16 _ui16Port;
                bool _bIsRunning;
                int _iConnectionRes;
        } *_pAsyncConnector;
};

inline void Mocket::setIdentifier (const char *pszIdentifier)
{
    _identifier = pszIdentifier;
}

inline const char * Mocket::getIdentifier (void)
{
    return _identifier;
}

inline int Mocket::registerPeerUnreachableWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
{
    _pPeerUnreachableWarningCallbackFn = pCallbackFn;
    _pPeerUnreachableCallbackArg = pCallbackArg;
    return 0;
}

inline int Mocket::registerPeerReachableCallback (PeerReachableCallbackFnPtr pCallbackFn, void *pCallbackArg)
{
    _pPeerReachableCallbackFn = pCallbackFn;
    _pPeerReachableCallbackArg = pCallbackArg;
    return 0;
}

inline int Mocket::registerSuspendReceivedWarningCallback (PeerUnreachableWarningCallbackFnPtr pCallbackFn, void *pCallbackArg)
{
    _pSuspendReceivedWarningCallbackFn = pCallbackFn;
    _pSuspendReceivedCallbackArg = pCallbackArg;
    return 0;
}

inline MocketStats * Mocket::getStatistics (void)
{
    return &_stats;
}

inline int Mocket::enableCrossSequencing (bool bEnable)
{
    _bEnableCrossSequencing = bEnable;
    return 0;
}

inline bool Mocket::isCrossSequencingEnabled (void)
{
    return _bEnableCrossSequencing;
}

inline uint32 Mocket::getRemoteAddress (void)
{
    return _ui32RemoteAddress;
}

inline uint16 Mocket::getRemotePort (void)
{
    return _ui16RemotePort;
}

inline uint32 Mocket::getLocalAddress (void)
{
    return _ui32LocalAddress;
}

inline uint16 Mocket::getLocalPort (void) const
{
    return _ui16LocalPort;
}

inline int Mocket::setConnectionLingerTime (uint32 ui32LingerTime)
{
    _ui32LingerTime = ui32LingerTime;
    return 0;
}

inline uint32 Mocket::getConnectionLingerTime (void)
{
    return _ui32LingerTime;
}

inline const char* Mocket::getStatsIP (void)
{
    return "127.0.0.1";
}

inline uint16 Mocket::getStatsPort (void)
{
    return DEFAULT_STATS_PORT;
}

inline uint16 Mocket::getStatsUpdateInverval (void)
{
    return DEFAULT_STATS_UPDATE_INTERVAL;
}

inline uint16 Mocket::getMTU (void)
{
    return _ui16MTU;
}

inline uint16 Mocket::getMaximumMTU (void)
{
    return MAXIMUM_MTU;
}

inline void Mocket::setMocketUUID (uint32 ui32MocketUUID)
{
    _ui32MocketUUID = ui32MocketUUID;
}

inline uint32 Mocket::getMocketUUID (void)
{
    return _ui32MocketUUID;
}

inline char * Mocket::getPassword (void)
{
    return _pszPassword;
}

inline uint16 Mocket::getPasswordLength (void)
{
    return DEFAULT_PASSWORD_LENGTH;
}

inline void Mocket::debugStateCapture (void)
{
    _bSendOnlyEvenPackets = true;
}

inline bool Mocket::isDebugStateCapture (void)
{
    return _bSendOnlyEvenPackets;
}

inline bool Mocket::isEncrypted(void) const
{
    return _bEnableDtls;
}

inline void Mocket::useTwoWayHandshake (void)
{
    _bUseTwoWayHandshake = true;
}

inline void Mocket::setKeepAliveTimeout (uint16 ui16Timeout)
{
    _ui16KeepAliveTimeout = ui16Timeout;
}

inline void Mocket::disableKeepAlive (void)
{
    _bUsingKeepAlive = false;
}

inline void Mocket::setInitialAssumedRTT (uint32 ui32RTT)
{
    _ui32InitialAssumedRTT = ui32RTT;
}

inline void Mocket::setMaximumRTO (uint32 ui32RTO)
{
    _ui32MaximumRTO = ui32RTO;
}

inline void Mocket::setMinimumRTO (uint32 ui32RTO)
{
    _ui32MinimumRTO = ui32RTO;
}

inline void Mocket::setRTOFactor (float fRTOFactor)
{
    _fRTOFactor = fRTOFactor;
}

inline void Mocket::setRTOConstant (uint16 ui16RTOConstant)
{
    _ui16RTOConstant = ui16RTOConstant;
}

inline void Mocket::disableRetransmitCountFactorInRTO (void)
{
    _bUseRetransmitCountInRTO = false;
}

inline void Mocket::setMaximumWindowSize (uint32 ui32WindowSize)
{
    _ui32MaximumWindowSize = ui32WindowSize;
}

inline void Mocket::setSAckTransmitTimeout (uint16 ui16SAckTransTO)
{
    _ui16SAckTransmitTimeout = ui16SAckTransTO;
}

inline void Mocket::setConnectTimeout (uint32 ui32ConnectTO)
{
    _ui32ConnectTimeout = ui32ConnectTO;
}

inline void Mocket::setUDPReceiveConnectionTimeout (uint16 ui16UDPRecConTO)
{
    _ui32UDPReceiveConnectionTimeout = ui16UDPRecConTO;
}

inline void Mocket::setUDPReceiveTimeout (uint16 ui16UDPRecTO)
{
    _ui32UDPReceiveTimeout = ui16UDPRecTO;
}

inline bool Mocket::usingFastRetransmit (void)
{
    return _bUsingFastRetransmit;
}
inline bool Mocket::usingRecBandEst (void)
{
    return _bUseReceiverSideBandwidthEstimation;
}

inline void Mocket::resetRemoteAddress (uint32 ui32NewRemoteAddress, uint16 ui16NewRemotePort)
{
    // Reset remote address in Mocket
    _ui32RemoteAddress = ui32NewRemoteAddress;
    _ui16RemotePort = ui16NewRemotePort;
}

inline void Mocket::setKeysExchanged (bool bValue)
{
    _bKeysExchanged = bValue;
}

inline bool Mocket::areKeysExchanged (void)
{
    return _bKeysExchanged;
}

inline void Mocket::setSupportReEstablish (bool bValue)
{
    _bSupportReEstablish = bValue;
}

inline bool Mocket::supportReEstablish (void)
{
    return _bSupportReEstablish;
}

inline StateMachine * Mocket::getStateMachine (void)
{
    return &_sm;
}

inline CommInterface * Mocket::getCommInterface (void)
{
    return _pCommInterface;
}

inline Receiver * Mocket::getReceiver (void)
{
    return _pReceiver;
}

inline Transmitter * Mocket::getTransmitter (void)
{
    return _pTransmitter;
}

inline PacketProcessor * Mocket::getPacketProcessor (void)
{
    return _pPacketProcessor;
}

inline ACKManager * Mocket::getACKManager (void)
{
    return &_ackManager;
}

inline CancelledTSNManager * Mocket::getCancelledTSNManager (void)
{
    return &_cancelledTSNManager;
}

inline MocketStatusNotifier * Mocket::getMocketStatusNotifier (void)
{
    return _pMocketStatusNotifier;
}

inline uint32 Mocket::getOutgoingValidation (void)
{
    if (_bOriginator) {
        return _stateCookie.getValidationA();
    }
    else {
        return _stateCookie.getValidationZ();
    }
}

inline uint32 Mocket::getIncomingValidation (void)
{
    if (_bOriginator) {
        return _stateCookie.getValidationZ();
    }
    else {
        return _stateCookie.getValidationA();
    }
}

inline StateCookie * Mocket::getStateCookie (void)
{
    return &_stateCookie;
}

inline int32 Mocket::getReceiveTimeout (void)
{
    return DEFAULT_RECEIVE_TIMEOUT;
}

inline uint16 Mocket::getUDPReceiveTimeout (void)
{
    return _ui32UDPReceiveTimeout;
}

inline uint16 Mocket::getKeepAliveTimeout (void)
{
    return _ui16KeepAliveTimeout;
}

inline bool Mocket::usingKeepAlive (void)
{
    return _bUsingKeepAlive;
}

inline uint32 Mocket::getInitialAssumedRTT (void)
{
    return _ui32InitialAssumedRTT;
}

inline uint32 Mocket::getMaximumRTO (void)
{
    return _ui32MaximumRTO;
}

inline uint32 Mocket::getMinimumRTO (void)
{
    return _ui32MinimumRTO;
}

inline float Mocket::getRTOFactor (void)
{
    return _fRTOFactor;
}

inline uint16 Mocket::getRTOConstant (void)
{
    return _ui16RTOConstant;
}

inline uint16 Mocket::getUnreliableSequencedDeliveryTimeout (void)
{
    return DEFAULT_UNRELIABLE_SEQUENCED_DELIVERY_TIMEOUT;
}

inline uint32 Mocket::getMaximumWindowSize (void)
{
    return _ui32MaximumWindowSize;
}

inline uint16 Mocket::getShutdownTimeout (void)
{
    return DEFAULT_SHUTDOWN_TIMEOUT;
}

inline uint16 Mocket::getSAckTransmitTimeout (void)
{
    return _ui16SAckTransmitTimeout;
}

inline uint16 Mocket::getCancelledTSNTransmitTimeout (void)
{
    return DEFAULT_CANCELLED_TSN_TRANSMIT_TIMEOUT;
}

inline uint32 Mocket::getTransmitRateLimit (void)
{
    return _ui32TransmitRateLimit;
}

inline uint32 Mocket::getBandEstMaxSamplesNumber (void)
{
    return _ui32BandEstMaxSamplesNumber;
}

inline uint32 Mocket::getBandEstTimeInterval (void)
{
    return _ui32BandEstTimeInterval;
}

inline uint64 Mocket::getBandEstSamplingTime (void)
{
    return _ui64BandEstSamplingTime;
}

inline uint16 Mocket::getMaxShutdownAttempts (void)
{
    return DEFAULT_MAX_SHUTDOWN_ATTEMPTS;
}

inline uint32 Mocket::getMaxIntervalBetweenRTTTimestamps (void)
{
    return DEFAULT_MAX_INTERVAL_BETWEEN_RTT_TIMESTAMPS;
}

inline uint32 Mocket::getMaxSuspendTimeout (void)
{
    return (uint32) (_ui32SuspendTimeout);
}

inline uint32 Mocket::getMaxFlushDataTimeout (void)
{
    return (uint32) (_ui32FlushDataTimeout);
}

inline uint32 Mocket::getMaxSendNewSuspendResumeTimeout (void)
{
    return DEFAULT_SEND_NEW_SUSPEND_RESUME_MSG_TIMEOUT;
}

inline uint16 Mocket::getInitialAssumedBandwidth (void)
{
    return _ui16InitialAssumedBandwidth;
}

inline uint32 Mocket::getTransmissionRateModulationInitialThreshold (void)
{
    return _ui32TransmissionRateModulationInitialThreshold;
}

inline void Mocket::packetProcessorTerminating (void)
{
    _termSync.packetProcessorTerminating();
}

inline void Mocket::receiverTerminating (void)
{
    _termSync.receiverTerminating();
}

inline void Mocket::transmitterTerminating (void)
{
    _termSync.transmitterTerminating();
}

#endif   // #ifndef INCL_MESSAGE_MOCKET_H
