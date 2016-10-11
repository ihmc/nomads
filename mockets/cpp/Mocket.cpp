/*
 * Mocket.cpp
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

#include "Mocket.h"

#include "ACKManager.h"
#include "MessageSender.h"
#include "MocketStatusNotifier.h"
#include "Packet.h"
#include "PacketProcessor.h"
#include "Receiver.h"
#include "Transmitter.h"
#include "UDPCommInterface.h"

#include "ConfigManager.h"
#include "UDPDatagramSocket.h"
#include "Logger.h"
#include "net/NetUtils.h"
#include "NLFLib.h"


#ifndef MOCKETS_NO_CRYPTO
using namespace CryptoUtils;
#endif

using namespace NOMADSUtil;

#define checkAndLogMsg if (pLogger) pLogger->logMsg

Mocket::Mocket (const char *pszConfigFile, CommInterface *pCI, bool bDeleteCIWhenDone)
    : _termSync (this)
{
    _bEnableCrossSequencing = false;
    _bOriginator = true;
    _ui16UDPBufferSize = DEFAULT_UDP_BUFFER_SIZE;
    _ui32RemoteAddress = 0;
    _ui16RemotePort = 0;
    if (pCI == NULL) {
        _pCommInterface = new UDPCommInterface (new UDPDatagramSocket(), true);
        _bLocallyCreatedCI = true;
        _bDeleteCIWhenDone = true;
    }
    else {
        _pCommInterface = pCI;
        _bLocallyCreatedCI = false;
        _bDeleteCIWhenDone = bDeleteCIWhenDone;
    }
    _pPacketProcessor = NULL;
    _pReceiver = NULL;
    _pTransmitter = NULL;

	// Initialize default settings
    _ui16MTU = DEFAULT_MTU;
    _ui32ConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
    _ui32UDPReceiveConnectionTimeout = DEFAULT_UDP_RECEIVE_CONNECTION_TIMEOUT;
    _ui32UDPReceiveTimeout = DEFAULT_UDP_RECEIVE_TIMEOUT;
    _ui16KeepAliveTimeout = DEFAULT_KEEP_ALIVE_TIMEOUT;
    _bUsingKeepAlive = true;
    _ui32InitialAssumedRTT = DEFAULT_INITIAL_ASSUMED_RTT;
    _ui32MaximumRTO = DEFAULT_MAXIMUM_RTO;
    _ui32MinimumRTO = DEFAULT_MINIMUM_RTO;
    if (_ui32MinimumRTO < DEFAULT_SACK_TRANSMIT_TIMEOUT) {
        // Avoid retransmission before the Ack can arrive
        _ui32MinimumRTO = DEFAULT_SACK_TRANSMIT_TIMEOUT;
    }
    _fRTOFactor = (float) DEFAULT_RTO_FACTOR;
    _ui16RTOConstant = DEFAULT_RTO_CONSTANT;
    _bUseRetransmitCountInRTO = true;
    _ui32MaximumWindowSize = DEFAULT_MAXIMUM_WINDOW_SIZE;
    _ui32LingerTime = DEFAULT_LINGER_TIME;
    _ui16SAckTransmitTimeout = DEFAULT_SACK_TRANSMIT_TIMEOUT;
    _bEnableXMitLogging = false;
    _bEnableRecvLogging = false;
    _bUseBandwidthEstimation = false;
    _ui32BandEstMaxSamplesNumber = DEFAULT_BAND_EST_MAX_SAMPLES_NUMBER;
    _ui32BandEstTimeInterval = DEFAULT_BAND_EST_TIME_INTERVAL;
    _ui64BandEstSamplingTime = DEFAULT_BAND_EST_SAMPLING_TIME;
    _ui16InitialAssumedBandwidth = DEFAULT_INITIAL_ASSUMED_BANDWIDTH;
    _bUseTwoWayHandshake = false;
    _ui32TransmissionRateModulationInitialThreshold = DEFAULT_TRANSMISSION_RATE_MODULATION_INITIAL_THRESHOLD;
    _ui32TransmitRateLimit = DEFAULT_TRANSMIT_RATE_LIMIT;
    _bUsingFastRetransmit = false;
    _bUseReceiverSideBandwidthEstimation = false;
    _bMocketAlreadyBound = false;
    _pPeerUnreachableWarningCallbackFn = NULL;
    _pPeerUnreachableCallbackArg = NULL;
    _pPeerReachableCallbackFn = NULL;
    _pPeerReachableCallbackArg = NULL;
    _pszCongestionControl = NULL;
    _pszNotificationIPAddress = NULL;
    _pszLocalAddress = NULL;

    if (pszConfigFile != NULL) {
        int rc = initParamsFromConfigFile(pszConfigFile);
        if (rc != 0) {
            checkAndLogMsg ("Mocket::Mocket", Logger::L_Warning, "failed to read config file <%s>; rc = %d\n", pszConfigFile, rc);
            // Should return an error code here, but cannot from the constructor
        }
    }
    
    _pMocketStatusNotifier = new MocketStatusNotifier();
    _pMocketStatusNotifier->init ((_pszNotificationIPAddress ? _pszNotificationIPAddress : getStatsIP()), getStatsPort(), false);
    
    _pKeyPair = NULL;
    _pSecretKey = NULL;
    _pszPassword = NULL;
    _ui32MocketUUID = 0;
    _bKeysExchanged = false;
    _bSupportReEstablish = false;
    _bSendOnlyEvenPackets = false;
}

Mocket::Mocket (StateCookie cookie, InetAddr *pRemoteAddr, const char *pszConfigFile, CommInterface *pCI, bool bDeleteCIWhenDone)
    : _termSync (this)
{
    _bEnableCrossSequencing = false;
    _bOriginator = false;
    _ui16UDPBufferSize = DEFAULT_UDP_BUFFER_SIZE;
    _ui32RemoteAddress = pRemoteAddr->getIPAddress();
    _ui16RemotePort = pRemoteAddr->getPort();
    _ui32LocalAddress = pCI->getLocalAddr().getIPAddress();
    _ui16LocalPort = pCI->getLocalPort();
    _pCommInterface = pCI;
    _bLocallyCreatedCI = false;
    _bDeleteCIWhenDone = bDeleteCIWhenDone;
    _pPacketProcessor = NULL;
    _pReceiver = NULL;
    _pTransmitter = NULL;
    _ui16MTU = DEFAULT_MTU;
    _ui32ConnectTimeout = DEFAULT_CONNECT_TIMEOUT;
    _ui32UDPReceiveConnectionTimeout = DEFAULT_UDP_RECEIVE_CONNECTION_TIMEOUT;
    _ui32UDPReceiveTimeout = DEFAULT_UDP_RECEIVE_TIMEOUT;
    _ui16KeepAliveTimeout = DEFAULT_KEEP_ALIVE_TIMEOUT;
    _bUsingKeepAlive = true;
    _ui32InitialAssumedRTT = DEFAULT_INITIAL_ASSUMED_RTT;
    _ui32MaximumRTO = DEFAULT_MAXIMUM_RTO;
    _ui32MinimumRTO = DEFAULT_MINIMUM_RTO;
    if (_ui32MinimumRTO < DEFAULT_SACK_TRANSMIT_TIMEOUT) {
        // Avoid retransmission before the Ack can arrive
        _ui32MinimumRTO = DEFAULT_SACK_TRANSMIT_TIMEOUT;
    }
    _fRTOFactor = (float) DEFAULT_RTO_FACTOR;
    _ui16RTOConstant = DEFAULT_RTO_CONSTANT;
    _bUseRetransmitCountInRTO = true;
    _ui32MaximumWindowSize = DEFAULT_MAXIMUM_WINDOW_SIZE;
    _ui32LingerTime = DEFAULT_LINGER_TIME;
    _ui16SAckTransmitTimeout = DEFAULT_SACK_TRANSMIT_TIMEOUT;
    _bEnableXMitLogging = false;
    _bEnableRecvLogging = false;
    _bUseBandwidthEstimation = false;
    _ui32BandEstMaxSamplesNumber = DEFAULT_BAND_EST_MAX_SAMPLES_NUMBER;
    _ui32BandEstTimeInterval = DEFAULT_BAND_EST_TIME_INTERVAL;
    _ui64BandEstSamplingTime = DEFAULT_BAND_EST_SAMPLING_TIME;
    _ui16InitialAssumedBandwidth = DEFAULT_INITIAL_ASSUMED_BANDWIDTH;
    _bUseTwoWayHandshake = false;
    _ui32TransmissionRateModulationInitialThreshold = DEFAULT_TRANSMISSION_RATE_MODULATION_INITIAL_THRESHOLD;
    _ui32TransmitRateLimit = DEFAULT_TRANSMIT_RATE_LIMIT;
    _bUsingFastRetransmit = false;
    _bUseReceiverSideBandwidthEstimation = false;
    _bMocketAlreadyBound = true;
    _pPeerUnreachableWarningCallbackFn = NULL;
    _pPeerUnreachableCallbackArg = NULL;
    _pPeerReachableCallbackFn = NULL;
    _pPeerReachableCallbackArg = NULL;
    _pszNotificationIPAddress = NULL;
    _pszLocalAddress = NULL;

    if (pszConfigFile != NULL) {
        int rc = initParamsFromConfigFile (pszConfigFile);
        if (rc != 0) {
            checkAndLogMsg ("Mocket::Mocket", Logger::L_Warning, "failed to read config file <%s>; rc = %d\n", pszConfigFile, rc);
            // Should return an error code here, but cannot from the constructor
	}
    }

    _pKeyPair = NULL;
    _pSecretKey = NULL;
    _pszPassword = NULL;
    _ui32MocketUUID = 0;
    _bKeysExchanged = false;
    _bSupportReEstablish = false;
    _bSendOnlyEvenPackets = false;
    _pszCongestionControl = NULL;

    _stateCookie = cookie;
    _sm.receivedCookieEcho();    // This will put the mocket into the established state
    _ackManager.init (cookie.getControlTSNA(),
                      cookie.getReliableSequencedTSNA(),
                      cookie.getReliableUnsequencedIDA());

    _pPacketProcessor = new PacketProcessor (this);      // Construct this before Receiver
    _pReceiver = new Receiver (this, _bEnableRecvLogging);
    _pTransmitter = new Transmitter (this, _bEnableXMitLogging);
    _pTransmitter->setTransmitRateLimit (_ui32TransmitRateLimit);
    _pMocketStatusNotifier = new MocketStatusNotifier();
    _pPacketProcessor->init();
    _pMocketStatusNotifier->init ((_pszNotificationIPAddress ? _pszNotificationIPAddress : getStatsIP()), getStatsPort(), false);
    
    InetAddr localAddr;
    if (_pszLocalAddress != NULL) {
        localAddr.setIPAddress (_pszLocalAddress);
    }
    else {
        localAddr = _pCommInterface->getLocalAddr();
    }
    _pMocketStatusNotifier->connectionReceived (getIdentifier(), &localAddr, _ui16LocalPort, pRemoteAddr, _ui16RemotePort);

    int rc = 0;
    if (0 != (rc = pCI->setReceiveBufferSize (_ui16UDPBufferSize))) {
        checkAndLogMsg ("Mocket::Mocket", Logger::L_MildError,
                        "could not set UDP receive buffer size to %d; rc = %d\n", (int) _ui16UDPBufferSize, rc);
        return;
    }
    // Starting of PacketProcessor, Receiver, and Transmitter threads has been moved into startThreads() method
}

Mocket::~Mocket (void)
{
    if (_sm.getCurrentState() == StateMachine::S_ESTABLISHED) {
        close();
    }
    if ((_pReceiver) && (_pTransmitter) && (_pPacketProcessor)) {
        checkAndLogMsg ("Mocket::delete", Logger::L_MediumDetailDebug, "call waitForComponentTermination\n");
        _termSync.waitForComponentTermination();
        checkAndLogMsg ("Mocket::delete", Logger::L_MediumDetailDebug, "waitForComponentTermination ended, delete threads\n");
        // NOTE: The packet processor must be deleted before the receiver - see comment in the destructor of PacketProcessor
        delete _pPacketProcessor;
        _pPacketProcessor = NULL;
        delete _pReceiver;
        _pReceiver = NULL;
        delete _pTransmitter;
        _pTransmitter = NULL;
    }
    delete _pMocketStatusNotifier;
    _pMocketStatusNotifier = NULL;

    if (_bDeleteCIWhenDone) {
        delete _pCommInterface;
    }
    _pCommInterface = NULL;

    if (_pKeyPair) {
        delete _pKeyPair;
        _pKeyPair = NULL;
    }
    if (_pSecretKey) {
        delete _pSecretKey;
        _pSecretKey = NULL;
    }
    if (_pszPassword) {
        delete[] _pszPassword;
        _pszPassword = NULL;
    }
}

void Mocket::startThreads (void)
{
    int res = 0;
    _pPacketProcessor->start();
    res = _pPacketProcessor->setPriority (10);
    if (res < 0) {
        checkAndLogMsg ("Mocket::startThreads", Logger::L_MildError,
                        "failed to set PacketProcessor thread priority; res = %d\n", res);
    }
    _pReceiver->start();
    res = _pReceiver->setPriority (10);
    if (res < 0) {
        checkAndLogMsg ("Mocket::startThreads", Logger::L_MildError,
                        "failed to set Receiver thread priority; res = %d\n", res);
    }
    _pTransmitter->start();
    res = _pTransmitter->setPriority (8);
    if (res < 0) {
        checkAndLogMsg ("Mocket::startThreads", Logger::L_MildError,
                        "failed to set Transmitter thread priority; res = %d\n", res);
    }
    if (_bUseBandwidthEstimation) {
        activateBandwidthEstimation();
    }
    if (_pszCongestionControl != NULL) {
        // Instantiate the correct subclass of CongestionControl
        activateCongestionControl();
    }
}

int Mocket::initParamsFromConfigFile (const char *pszConfigFile)
{
    int rc;
    ConfigManager cm;
    if (0 != (rc = cm.init())) {
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_MildError,
                        "could not initialize the ConfigManager; rc = %d\n", rc);
        return -1;
    }
    if (0 != (rc = cm.readConfigFile (pszConfigFile, true))) {
		checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
			            "failed to read mocket config params from %s; rc = %d\n", pszConfigFile, rc);
		return -2;
    }

    if (cm.hasValue ("MTU")) {
        uint16 ui16 = (uint16) cm.getValueAsInt ("MTU");
        if (ui16 <= MAXIMUM_MTU) {
            _ui16MTU = ui16;
            checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                            "set MTU to %d\n", (int) _ui16MTU);
        }
        else {
            checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_Warning,
                            "not using MTU setting of %d in config file since it is larger than the maximum MTU of %d\n",
                            (int) ui16, (int) MAXIMUM_MTU);
        }
    }
    if (cm.hasValue ("ConnectTimeout")) {
        _ui32ConnectTimeout = (uint32) cm.getValueAsInt ("ConnectTimeout");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set connect timeout to %d\n", (int) _ui32ConnectTimeout);
    }
    if (cm.hasValue ("UDPReceiveConnectionTimeout")) {
        _ui32UDPReceiveConnectionTimeout = (uint16) cm.getValueAsInt ("UDPReceiveConnectionTimeout");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set connect receive timeout to %d\n", (int) _ui32UDPReceiveConnectionTimeout);
    }
    if (cm.hasValue ("UDPReceiveTimeout")) {
        _ui32UDPReceiveTimeout = (uint16) cm.getValueAsInt ("UDPReceiveTimeout");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set UDP receive timeout to %d\n", (int) _ui32UDPReceiveTimeout);
    }
    if (cm.hasValue ("KeepAliveTimeout")) {
        _ui16KeepAliveTimeout = (uint16) cm.getValueAsInt ("KeepAliveTimeout");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set keep alive timeout to %d\n", (int) _ui16KeepAliveTimeout);
    }
    if (cm.hasValue ("InitialAssumedRTT")) {
        _ui32InitialAssumedRTT = (uint32) cm.getValueAsInt ("InitialAssumedRTT");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set initial assumed RTT to %d\n", (int) _ui32InitialAssumedRTT);
    }
    if (cm.hasValue ("MaximumRTO")) {
        _ui32MaximumRTO = (uint32) cm.getValueAsInt ("MaximumRTO");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set maximum RTO to %d\n", (int) _ui32MaximumRTO);
    }if (cm.hasValue ("MinimumRTO")) {
        _ui32MinimumRTO = (uint32) cm.getValueAsInt ("MinimumRTO");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set minimum RTO to %d\n", (int) _ui32MinimumRTO);
    }
    if (cm.hasValue ("RTOFactor")) {
        _fRTOFactor = (float) atof (cm.getValue ("RTOFactor"));
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set RTOFactor to %.2f\n", _fRTOFactor);
    }
    if (cm.hasValue ("MaximumWindowSize")) {
        _ui32MaximumWindowSize = (uint32) cm.getValueAsInt ("MaximumWindowSize");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set Maximum Window Size to %lu\n", _ui32MaximumWindowSize);
    }
    if (cm.hasValue ("SAckTransmitTimeout")) {
        _ui16SAckTransmitTimeout = (uint16) cm.getValueAsInt ("SAckTransmitTimeout");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set SAck transmit timeout to %d\n", (int) _ui16SAckTransmitTimeout);
    }
    if (cm.hasValue ("TransmitRateLimit")) {
        _ui32TransmitRateLimit = (uint32) cm.getValueAsInt ("TransmitRateLimit");
        if (_pTransmitter) {
            _pTransmitter->setTransmitRateLimit (_ui32TransmitRateLimit);
        }
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set transmit rate limit to %lu bytes/sec\n", _ui32TransmitRateLimit);
    }
    if (cm.hasValue ("EnableXMitLogging")) {
        _bEnableXMitLogging = cm.getValueAsBool ("EnableXMitLogging");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set transmit logging to %s\n", _bEnableXMitLogging ? "true" : "false");
    }
    if (cm.hasValue ("EnableRecvLogging")) {
        _bEnableRecvLogging = cm.getValueAsBool ("EnableRecvLogging");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set receive logging to %s\n", _bEnableRecvLogging ? "true" : "false");
    }
    if (cm.hasValue ("UseBandwidthEstimation")) {
        _bUseBandwidthEstimation = cm.getValueAsBool ("UseBandwidthEstimation");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set use bandwidth estimation to %s\n", _bUseBandwidthEstimation ? "true" : "false");
    }
    if (cm.hasValue ("UseCongestionControl")) {
        _pszCongestionControl = strDup (cm.getValue ("UseCongestionControl"));
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "using %s congestion control mechanism\n", _pszCongestionControl);
    }
    if (cm.hasValue ("InitialAssumedBandwidth")) {
        _ui16InitialAssumedBandwidth = (uint16) cm.getValueAsInt ("InitialAssumedBandwidth");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set initial assumed bandwidth to %d\n", (int) _ui16InitialAssumedBandwidth);
    }
    if (cm.hasValue ("UseTwoWayHandshake")) {
        _bUseTwoWayHandshake = cm.getValueAsBool ("UseTwoWayHandshake");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set use two way handshake to %s\n", _bUseTwoWayHandshake ? "true" : "false");
    }
    if (cm.hasValue ("TransmissionRateModulationInitialThreshold")) {
        _ui32TransmissionRateModulationInitialThreshold = (uint16) cm.getValueAsInt ("TransmissionRateModulationInitialThreshold");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set transmission rate modulation initial bandwidth threashold to %d\n", (int) _ui32TransmissionRateModulationInitialThreshold);
    }
    if (cm.hasValue ("FastRetransmit")) {
        _bUsingFastRetransmit = cm.getValueAsBool ("FastRetransmit");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set fast retransmit to %s\n", _bUsingFastRetransmit ? "true" : "false");
    }
    if (cm.hasValue ("UseReceiverSideBandwidthEstimation")) {
        _bUseReceiverSideBandwidthEstimation = cm.getValueAsBool ("UseReceiverSideBandwidthEstimation");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set use receiver side bandwidth estimation to %s\n", _bUseReceiverSideBandwidthEstimation ? "true" : "false");
    }
    if (cm.hasValue ("DisableKeepAlive")) {
        bool temp = cm.getValueAsBool ("DisableKeepAlive");
        if (temp) {
            _bUsingKeepAlive = false;
            checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                            "disable keepAlives\n");
        }
    }
    if (cm.hasValue ("DisableRetransmitCountFactorInRTO")) {
        bool temp = cm.getValueAsBool ("DisableRetransmitCountFactorInRTO");
        if (temp) {
            _bUseRetransmitCountInRTO = false;
            checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                            "disable retransmit count in RTO\n");
        }
    }
    if (cm.hasValue ("RTOConstant")) {
        _ui16RTOConstant = (uint16)cm.getValueAsInt ("RTOConstant");
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "set RTO constant to %d\n", (int) _ui16RTOConstant);
    }
    if (cm.hasValue ("StatusNotificationAddress")) {
        _pszNotificationIPAddress = strDup (cm.getValue("StatusNotificationAddress"));       
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "Status notification address %s\n", (_pszNotificationIPAddress ? _pszNotificationIPAddress : " NULL"));
    }
    if (cm.hasValue ("LocalAddress")) {
        _pszLocalAddress = strDup (cm.getValue("LocalAddress"));       
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_LowDetailDebug,
                        "Local address %s\n", (_pszLocalAddress ? _pszLocalAddress : " NULL"));
    }
    
    if (_ui32MinimumRTO < _ui16SAckTransmitTimeout) {
        _ui32MinimumRTO = _ui16SAckTransmitTimeout;
        checkAndLogMsg ("Mocket::initParamsFromConfigFile", Logger::L_MildError,
                        "minimum RTO was set to a valuer than the SAck transmit timeout, reset to %d\n", _ui32MinimumRTO);
    }

    return 0;
}

int Mocket::bind (const char *pszBindAddr, uint16 ui16BindPort)
{
    InetAddr bindAddr (NetUtils::getHostByName (pszBindAddr), ui16BindPort);
    int rc = _pCommInterface->bind (&bindAddr);
    if (rc == 0) {
        _bMocketAlreadyBound = true;
    }
    else {
        checkAndLogMsg ("Mocket::bind", Logger::L_MildError,
                        "could not bind CommInterface to address <%s> and port %d; rc = %d\n",
                        pszBindAddr?pszBindAddr:"<null>", (int) ui16BindPort, rc);
    }

    return rc;
}

int Mocket::connect (const char *pszRemoteHost, uint16 ui16RemotePort)
{
    return connect (pszRemoteHost, ui16RemotePort, false, 0);
}

int Mocket::connect (const char *pszRemoteHost, uint16 ui16RemotePort, int64 i64Timeout)
{
    return connect (pszRemoteHost, ui16RemotePort, false, i64Timeout);
}

int Mocket::connectAsync (const char *pszRemoteHost, uint16 ui16RemotePort)
{
    _pAsyncConnector = new AsynchronousConnector (this, pszRemoteHost, ui16RemotePort);
    _pAsyncConnector->start();
    return 0;
}

int Mocket::finishConnect (void)
{
    if (_sm.getCurrentState() == StateMachine::S_ESTABLISHED) {
        return 1;
    }
    if (!_pAsyncConnector->isRunning()) {
        // If the asynchronous connector thread is not running the connect operation ended
        // and if the mocket is not in the established state it means that the connection attempt failed
        return _pAsyncConnector->connectionRes();
    }
    return 0;
}

int Mocket::connect (const char *pszRemoteHost, uint16 ui16RemotePort, bool bPreExchangeKeys, int64 i64Timeout)
{
    if (_sm.getCurrentState() != StateMachine::S_CLOSED) {
        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                        "mocket must be in the closed state before attempting a connection; current state is %s\n",
                        _sm.getCurrentStateAsString());
        return -1;
    }
    if (pszRemoteHost == NULL) {
        return -2;
    }
    uint16 ui16ConnectPort = ui16RemotePort;
    uint32 ui32ConnectAddress = NetUtils::getHostByName (pszRemoteHost);
    if (ui32ConnectAddress == 0) {
        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                        "could not lookup IP address for host <%s>\n", pszRemoteHost);
        return -3;
    }

    checkAndLogMsg ("Mocket::connect", Logger::L_LowDetailDebug,
                    "attempting to open a connection to <%s>:%d\n", pszRemoteHost, ui16RemotePort);

    int64 i64AttemptEndTime = getTimeInMilliseconds();
    if (i64Timeout > 0) {
        i64AttemptEndTime += i64Timeout;
    }
    else {
        i64AttemptEndTime += _ui32ConnectTimeout;
        checkAndLogMsg ("Mocket::connect", Logger::L_LowDetailDebug,
                        "set connection timeout to default value of %d\n", _ui32ConnectTimeout);
    }

    int rc;
    
    if (!_bMocketAlreadyBound) {
        InetAddr bindAddr;
        if (0 != (rc = _pCommInterface->bind (&bindAddr))) {
            checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                            "could not initialize UDPDatagramSocket; rc = %d\n", rc);
            return -4;
        }
    }

    _ui32LocalAddress = _pCommInterface->getLocalAddr().getIPAddress();
    _ui16LocalPort = _pCommInterface->getLocalPort();
    
    if (0 != (rc = _pCommInterface->setReceiveBufferSize (_ui16UDPBufferSize))) {
        checkAndLogMsg ("Mocket::connect", Logger::L_Warning,
                        "could not set UDP receive buffer size to %d; rc = %d\n", (int) _ui16UDPBufferSize, rc);
    }
    if (0 != (rc = _pCommInterface->setReceiveTimeout (_ui32UDPReceiveConnectionTimeout))) {
        checkAndLogMsg ("Mocket::connect", Logger::L_Warning,
                        "could not set UDP timeout to %d; rc = %d\n", _ui32UDPReceiveConnectionTimeout, rc);
    }
    else {
        checkAndLogMsg ("Mocket::connect", Logger::L_Info,
                        "set UDP timeout to %d\n", _ui32UDPReceiveConnectionTimeout);
    }

    StateCookie stateCookie;

    if (!_bUseTwoWayHandshake) {
        // Create the Init packet
        uint32 ui32OutgoingValidation = (uint32) rand();
        Packet initPacket (this);
        initPacket.setValidation (ui32OutgoingValidation);
        initPacket.setWindowSize (getMaximumWindowSize());
        initPacket.setSequenceNum (0);
        initPacket.addInitChunk (ui32OutgoingValidation, 0, 0, 0, 0, 0);
        // Advance state machine
        _sm.associate();

        // Send the Init packet and wait for InitAck
        char achReplyBuf [MAXIMUM_MTU];
        while (true) {
            int rc;
            InetAddr remoteAddr;
            InetAddr sendToAddr (ui32ConnectAddress, ui16ConnectPort);
            rc = _pCommInterface->sendTo (&sendToAddr, initPacket.getPacket(), initPacket.getPacketSize());
            checkAndLogMsg ("Mocket::connect", Logger::L_LowDetailDebug,
                            "sent init packet\n");
            if (rc < 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "sending packet failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            else {
                rc = _pCommInterface->receive (achReplyBuf, MAXIMUM_MTU, &remoteAddr);
                if (rc < 0) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "receive failed with rc = %d; os error = %d\n",
                                    rc, _pCommInterface->getLastError());
                }
                else if (rc == 0) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "timed out waiting for reply on Init packet from: %s:%d\n",
                                    pszRemoteHost, ui16ConnectPort);
                }
                else if (rc <= Packet::HEADER_SIZE) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "received a short packet; rc = %d\n", rc);
                }
                else if ((remoteAddr.getIPAddress() != ui32ConnectAddress) || (remoteAddr.getPort() != ui16ConnectPort)) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "received a packet from a different endpoint %s:%d; expecting one from %s:%d\n",
                                    (char*)remoteAddr.getIPAsString(), (int) remoteAddr.getPort(),
                                    (char*)InetAddr(ui32ConnectAddress).getIPAsString(), (int) ui16ConnectPort);
                }
                else {
                    // Parse the packet and look for the InitAck chunk
                    Packet replyPacket (achReplyBuf, rc);
                    if (replyPacket.getChunkType() != Packet::CT_InitAck) {
                        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                        "received a reply packet without the InitAck chunk (type %d) as the first chunk; chunk type is %d\n",
                                        (int) Packet::CT_InitAck, (int) replyPacket.getChunkType());
                    }
                    else {
                        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                        "Received packet with init_ack\n");
                        // Found the InitAck chunk
                        InitAckChunkAccessor accessor = replyPacket.getInitAckChunk();
                        stateCookie = accessor.getStateCookie();
                        if (!_sm.receivedInitAck()) {
                            checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                            "mocket state machine in illegal state\n");
                            _sm.abort();
                            return -7;
                        }
                        break;
                    }
                }
            }
            if (getTimeInMilliseconds() > i64AttemptEndTime) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "connection timeout while waiting for InitAck\n");
                _sm.abort();

                InetAddr localAddr;
                if (_pszLocalAddress != NULL) {
                    localAddr.setIPAddress (_pszLocalAddress);
                }
                else {
                    localAddr = _pCommInterface->getLocalAddr();
                }

                InetAddr connectToAddr (ui32ConnectAddress);
                _pMocketStatusNotifier->connectionFailed (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &connectToAddr, ui16ConnectPort);
                return -8;
            }

            // Keep trying until the timeout, but sleep for a short interval to make sure we don't hog the CPU
            sleepForMilliseconds (1000);
        }

        // Create the CookieEcho packet
        Packet mpCookieEcho (this);
        mpCookieEcho.setValidation (ui32OutgoingValidation);
        mpCookieEcho.setWindowSize (getMaximumWindowSize());
        mpCookieEcho.setSequenceNum (0);
        mpCookieEcho.addCookieEchoChunk (&stateCookie);

        if (bPreExchangeKeys) {
            #ifdef MOCKETS_NO_CRYPTO
                checkAndLogMsg ("Mocket::connect", Logger::L_SevereError,
                                "crypto disabled at build time\n");
                _mSuspend.unlock();
                return -9;
            #else
                // Insert the public key in the packet to support end-to-end connectivity
                // Create a new pair of keys
                _pKeyPair = new PublicKeyPair();
                if (0 != _pKeyPair->generateNewKeyPair()) {
                    // Error while creating publicKeyPair
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "error while creating publicKeyPair\n");
                    _mSuspend.unlock();
                    return -3;
                }
                PublicKey::KeyData *pKeyData = _pKeyPair->getPublicKey()->getKeyAsDEREncodedX509Data();
                mpCookieEcho.addPublicKey (pKeyData->getData(), pKeyData->getLength());
                delete pKeyData;
            #endif
        }
        else {
            mpCookieEcho.addPublicKey (NULL, (uint32) 0);
        }

        // Send the CookieEcho packet and wait for CookieAck
        while (true) {
            int rc;
            InetAddr remoteAddr;
            InetAddr sendToAddr (ui32ConnectAddress, ui16ConnectPort);
            rc = _pCommInterface->sendTo (&sendToAddr, mpCookieEcho.getPacket(), mpCookieEcho.getPacketSize());
            checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                            "sent cookie_echo packet\n");
            if (rc < 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "send on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
                // Keep trying until the timeout, but sleep for a short interval to make sure we don't hog the CPU
                sleepForMilliseconds (1000);
            }
            if (bPreExchangeKeys) {
                // Pre-exchanging keys adds ovehead to connect operation
                // The time it takes to create the keys and perform cryptation
                // and decryptation of the data is highly dependent on the system
                // performance and the system load. A higher timeout for the socket
                // receive operation needs to be set to avoid repeated send of the
                // cookie-echo
                // NOTE: no need to restore the socket timeout once the keys have
                // been exchanged, since it will be set in receiver.
                _pCommInterface->setReceiveTimeout (DEFAULT_PRE_EXCHANGE_KEYS_CONNECT_RECEIVE_TIMEOUT);
            }
            rc = _pCommInterface->receive (achReplyBuf, MAXIMUM_MTU, &remoteAddr);
            if (rc < 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "receive on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            else if (rc == 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "timed out waiting for reply on CookieAck packet\n");
            }
            else if (rc <= Packet::HEADER_SIZE) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "received a short packet; rc = %d\n", rc);
            }
            else if ((remoteAddr.getIPAddress() != ui32ConnectAddress) || (remoteAddr.getPort() != ui16ConnectPort)) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "received a packet from a different endpoint %s:%d; expecting one from %s:%d\n",
                                (char*)remoteAddr.getIPAsString(), (int) remoteAddr.getPort(),
                                (char*)InetAddr(ui32ConnectAddress).getIPAsString(), (int) ui16ConnectPort);
            }
            else {
                // Parse the packet and look for the CookieAck chunk
                Packet replyPacket (achReplyBuf, rc);
                if (replyPacket.getChunkType() != Packet::CT_CookieAck) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "received a reply packet without the CookieAck chunk as the first chunk; chunk type is %d\n",
                                    (int) replyPacket.getChunkType());
                }
                else {
                    checkAndLogMsg ("Mocket::connect", Logger::L_Info,
                                    "received a valid CookieAck packet from %s:%d - proceeding with the connection\n",
                    (char*)remoteAddr.getIPAsString(), (int) remoteAddr.getPort());

                    // Found the CookieAck chunk
                    CookieAckChunkAccessor accessor = replyPacket.getCookieAckChunk();
                    uint16 ui16RemotePort = accessor.getPort();
                    if (!_sm.receivedCookieAck()) {
                        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                        "mocket state machine in illegal state\n");
                        _sm.abort();
                        return -10;
                    }
                    if (bPreExchangeKeys) {
                        #ifdef MOCKETS_NO_CRYPTO
                            checkAndLogMsg ("Mocket::connect", Logger::L_SevereError,
                                            "crypto disabled at build time\n");
                        #else
                            //**// Extract, decrypt and save UUID and password to create Ks
                            // Extract
                            uint32 ui32EncryptedDataLength = accessor.getEncryptedDataLength();
                            const char *pDataBuff = accessor.getEncryptedData();

                            // Decrypt
                            uint32 ui32ReceivedDataLength;
                            void *pReceivedData;
                            pReceivedData = CryptoUtils::decryptDataUsingPrivateKey (_pKeyPair->getPrivateKey(), pDataBuff, ui32EncryptedDataLength, &ui32ReceivedDataLength);

                            // Extract and save the password
                            uint32 ui32BuffPos = 0;
                            char pwd[9];
                            memcpy (pwd, pReceivedData, 9);
                            setPassword (pwd);
                            ui32BuffPos += 9;
                            // Reconstuct the secret key from the password
                            newSecretKey(_pszPassword);

                            // Extract and save the UUID
                            uint32 ui32UUID = *((uint32*)(((char*)pReceivedData) + ui32BuffPos));
                            setMocketUUID (ui32UUID);
                            // security information exchanged, save the information so we don't need to exchange new values for a suspension
                            // and we can support reEstablishConn
                            setKeysExchanged (true);
                            // Set that Ks and the UUID have been set so this side of the mocket supports abrupt disconnections
                            setSupportReEstablish (true);

                            free (pReceivedData);
                        #endif
                    }
                    _ui32RemoteAddress = ui32ConnectAddress;
                    _ui16RemotePort = ui16RemotePort;
                    break;
                }
            }
            if (getTimeInMilliseconds() > i64AttemptEndTime) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "connection timeout while waiting for CookieAck\n");
                _sm.abort();
                InetAddr localAddr;
                if (_pszLocalAddress != NULL) {
                    localAddr.setIPAddress (_pszLocalAddress);
                }
                else {
                    localAddr = _pCommInterface->getLocalAddr();
                }
                InetAddr connectToAddr (ui32ConnectAddress);
                _pMocketStatusNotifier->connectionFailed (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &connectToAddr, ui16ConnectPort);
                return -11;
            }

            // Keep trying until the timeout, but sleep for a short interval to make sure we don't hog the CPU
            sleepForMilliseconds (1000);
        }
    }
    // Use two way handshake connection mechanism
    else {
        // Create the SimpleConnect packet
        uint32 ui32OutgoingValidation = (uint32) rand();
        Packet simpleConnectPacket (this);
        simpleConnectPacket.setValidation (ui32OutgoingValidation);
        simpleConnectPacket.setWindowSize (getMaximumWindowSize());
        simpleConnectPacket.setSequenceNum (0);
        simpleConnectPacket.addSimpleConnectChunk (ui32OutgoingValidation, 0, 0, 0, 0, 0);
        // Advance state machine
        _sm.simpleConnect();

        // Send the SimpleConnect packet and wait for SimpleConnectAck
        char achReplyBuf [MAXIMUM_MTU];
        while (true) {
            int rc;
            InetAddr remoteAddr;
            InetAddr sendToAddr (ui32ConnectAddress, ui16ConnectPort);
            rc = _pCommInterface->sendTo (&sendToAddr, simpleConnectPacket.getPacket(), simpleConnectPacket.getPacketSize());
            if (rc < 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "send on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            rc = _pCommInterface->receive (achReplyBuf, MAXIMUM_MTU, &remoteAddr);
            if (rc < 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "receive on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            else if (rc == 0) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "timed out waiting for reply on simpleConnect packet from: %s:%d\n",
                                pszRemoteHost, ui16ConnectPort);
            }
            else if (rc <= Packet::HEADER_SIZE) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "received a short packet; rc = %d\n", rc);
            }
            else if ((remoteAddr.getIPAddress() != ui32ConnectAddress) || (remoteAddr.getPort() != ui16ConnectPort)) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "received a packet from a different endpoint %s:%d; expecting one from %s:%d\n",
                                (char*)remoteAddr.getIPAsString(), (int) remoteAddr.getPort(),
                                (char*)InetAddr(ui32ConnectAddress).getIPAsString(), (int) ui16ConnectPort);
            }
            else {
                // Parse the packet and look for the simpleConnectAck chunk
                Packet replyPacket (achReplyBuf, rc);
                if (replyPacket.getChunkType() != Packet::CT_SimpleConnectAck) {
                    checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                    "received a reply packet without the SimpleConnectAck chunk (type %d) as the first chunk; chunk type is %d\n",
                                    (int) Packet::CT_SimpleConnectAck, (int) replyPacket.getChunkType());
                }
                else {
                    // Found the SimpleConnectAck chunk
                    SimpleConnectAckChunkAccessor accessor = replyPacket.getSimpleConnectAckChunk();
                    stateCookie = accessor.getStateCookie();
                    _ui32RemoteAddress = ui32ConnectAddress;
                    _ui16RemotePort = accessor.getRemotePort();
                    if (!_sm.receivedSimpleConnectAck()) {
                        checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                        "mocket state machine in illegal state\n");
                        _sm.abort();
                        return -12;
                    }
                    break;
                }
            }
            if (getTimeInMilliseconds() > i64AttemptEndTime) {
                checkAndLogMsg ("Mocket::connect", Logger::L_MildError,
                                "connection timeout while waiting for SimpleConnectAck\n");
                _sm.abort();
                InetAddr localAddr;
                if (_pszLocalAddress != NULL) {
                    localAddr.setIPAddress (_pszLocalAddress);
                }
                else {
                    localAddr = _pCommInterface->getLocalAddr();
                }
                InetAddr connectToAddr (ui32ConnectAddress);
                _pMocketStatusNotifier->connectionFailed (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &connectToAddr, ui16ConnectPort);
                return -13;
            }

            // Keep trying until the timeout, but sleep for a short interval to make sure we don't hog the CPU
            sleepForMilliseconds (1000);
        }
    }
    
    // If the client is behind NAT send a message to punch a hole for the peer Mocket opened on the server side
    //TODO

    // Connected - Call the Status Notifier
    InetAddr localAddr;
    if (_pszLocalAddress != NULL) {
        localAddr.setIPAddress (_pszLocalAddress);
    }
    else {
        localAddr = _pCommInterface->getLocalAddr();
    }
    InetAddr remoteAddr (_ui32RemoteAddress);
    _pMocketStatusNotifier->connected (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &remoteAddr, _ui16RemotePort);
    
    // Initialize the mocket
    _stateCookie = stateCookie;
    _ackManager.init (stateCookie.getControlTSNA(),
                      stateCookie.getReliableSequencedTSNA(),
                      stateCookie.getReliableUnsequencedIDA());
    _pPacketProcessor = new PacketProcessor (this);      // Construct this before Receiver
    _pTransmitter = new Transmitter (this, _bEnableXMitLogging);
    _pTransmitter->setTransmitRateLimit (_ui32TransmitRateLimit);
    _pReceiver = new Receiver (this, _bEnableRecvLogging);
    _pPacketProcessor->init();
    startThreads();
    return 0;
}

bool Mocket::isConnected (void)
{
    return (_sm.getCurrentState() == StateMachine::S_ESTABLISHED);
}

int Mocket::reEstablishConn (uint32 ui32ReEstablishTimeout)
{
    #ifdef MOCKETS_NO_CRYPTO
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_SevereError,
                        "built without crypto support, illegal reconnection attempt\n");
        return -1;
    #else
    if ((!supportReEstablish()) || (!_pSecretKey) || (_ui32MocketUUID == 0)) {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                        "keys and UUID non exchanged: illegal reconnection attempt\n");
        return -1;
    }
    else {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                        "UUID = %d\n", _ui32MocketUUID);
    }
    // Set the start time
    uint64 ui64StartTime = getTimeInMilliseconds();
    // Check the parameter ReEstablishTimeout
    if (ui32ReEstablishTimeout < DEFAULT_MIN_RESUME_TIMEOUT) {
        //The time to wait is too short TODO: new state???
        ui32ReEstablishTimeout = DEFAULT_MIN_RESUME_TIMEOUT;
    }
    
    int rc;
    InetAddr bindAddr;
    if (0 != (rc = _pCommInterface->bind (&bindAddr))) {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                        "could not initialize DatagramSocket; rc = %d\n", rc);
        return -3;
    }
    _ui32LocalAddress = _pCommInterface->getLocalAddr().getIPAddress();
    _ui16LocalPort = _pCommInterface->getLocalPort();
    //printf ("Mocket::reEstablishConn * LOCAL during resume ip %lu * port %d *\n", _ui32LocalAddress, _ui16LocalPort);

    if (0 != (rc = _pCommInterface->setReceiveBufferSize (_ui16UDPBufferSize))) {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                        "could not set UDP receive buffer size to %d; rc = %d\n", (int) _ui16UDPBufferSize, rc);
        return -4;
    }
    if (0 != (rc = _pCommInterface->setReceiveTimeout (_ui32UDPReceiveConnectionTimeout))) {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                        "could not set UDP timeout to %d; rc = %d\n", _ui32UDPReceiveConnectionTimeout, rc);
        return -5;
    }
    
    // Create the reEstablish packet
    Packet reestablishPacket (this);
    reestablishPacket.setValidation (getOutgoingValidation());

    // Encrypt nonce (_ui32MocketUUID), local IP address and port with Ks
    char pchBuff[10]; // 4 dimension of _ui32MocketUUID + 4 dimension of _ui32LocalAddress + 2 dimension of _ui16LocalPort
    uint32 ui32BuffSize = 0;
    // Insert the UUID
    *((uint32*)(pchBuff)) = getMocketUUID();
    ui32BuffSize += 4;
    //TODO the local IP is 0!!
    // Insert the local IP address
    *((uint32*)(pchBuff+ui32BuffSize)) = _ui32LocalAddress;
    ui32BuffSize += 4;
    // Insert the local port
    *((uint16*)(pchBuff+ui32BuffSize)) = _ui16LocalPort;
    ui32BuffSize += 2;
    
    // Encrypt
    uint32 ui32EncryptedDataLen = 0;
    void *pEncryptedData = encryptDataUsingSecretKey (_pSecretKey, pchBuff, ui32BuffSize, &ui32EncryptedDataLen);
    if (pEncryptedData == NULL) {
        checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError, 
                         "unable to encrypt nonce with secret key\n");
        return -6;
    }
    if (reestablishPacket.addReEstablishChunk (pEncryptedData, ui32EncryptedDataLen)) {
        return -7;
    }
    
    // Send reEstablish and wait for reEstablish_ack
    uint64 ui64ReEstablishSent = 0;
    char achReplyBuf [MAXIMUM_MTU];
    while (true) {
        InetAddr remoteAddr;
        int rc;
        if ((getTimeInMilliseconds() - ui64ReEstablishSent) > getMaxSendNewSuspendResumeTimeout()) {
            // Send resume
            InetAddr remoteAddr (_ui32RemoteAddress, _ui16RemotePort);
            rc = _pCommInterface->sendTo (&remoteAddr, reestablishPacket.getPacket(), reestablishPacket.getPacketSize());
            if (rc < 0) {
                checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                                "send on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            else {
                ui64ReEstablishSent = getTimeInMilliseconds();
            }
        }
        // Wait for reEstablish_ack
        //printf ("Mocket::reEstablishConn Wait for reEstablish_ack\n");
        rc = _pCommInterface->receive (achReplyBuf, MAXIMUM_MTU, &remoteAddr);
        if (rc < 0) {
            checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                            "receive on datagram socket failed with rc = %d; os error = %d\n",
                            rc, _pCommInterface->getLastError());
        }
        else if (rc == 0) {
            checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                            "timed out waiting for reply on Resume packet from: %s:%d\n", 
                            (const char*) remoteAddr.getIPAsString(), remoteAddr.getPort());
        }
        else if (rc <= Packet::HEADER_SIZE) {
            checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                            "received a short packet; rc = %d\n", rc);
        }
        else if ((remoteAddr.getIPAddress() != _ui32RemoteAddress) || (remoteAddr.getPort() != _ui16RemotePort)) {
            checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                            "received a packet from a different endpoint %s:%d\n",
                            (char*)remoteAddr.getIPAsString(), remoteAddr.getPort());
        }
        else {
            // Parse the packet and look for the ReEstablishAck chunk
            Packet replyPacket (achReplyBuf, rc);
            if (replyPacket.getChunkType() != Packet::CT_ReEstablishAck) {
                checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                                "received a reply packet without the ReEstablishAck chunk (type %d) as the first chunk; chunk type is %d\n",
                                (int) Packet::CT_ReEstablishAck, (int) replyPacket.getChunkType());
            }
            else {
                // TODO: What if the ReEstablishAck gets lost, and we receive a message from the peer,
                // indicating it did receive the ReEstablish message, we should just assume the ack
                // was lost and restore the communication
                //
                // Found the ReEstablishAck chunk go in ESTABLISHED state
                // TODO: new state while waiting for the ReEstablishAck?
                break;
            }
        }
        // Check if the timeout is expired
        if ((uint64) getTimeInMilliseconds() > (ui64StartTime + (uint64) ui32ReEstablishTimeout)) {
            checkAndLogMsg ("Mocket::reEstablishConn", Logger::L_MildError,
                            "resume timeout expired while waiting for ResumeAck\n");
            _sm.abort();
            InetAddr localAddr;
            if (_pszLocalAddress != NULL) {
                localAddr.setIPAddress (_pszLocalAddress);
            }
            else {
                localAddr = _pCommInterface->getLocalAddr();
            }
            InetAddr connectToAddr (_ui32RemoteAddress);
            _pMocketStatusNotifier->connectionFailed (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &connectToAddr, _ui16RemotePort);
            return -10;
        }
    }
    
    return 0;
#endif
}

int Mocket::resumeAndRestoreState (Reader *pr, uint32 ui32ResumeTimeout)
{
#ifdef MOCKETS_NO_CRYPTO
    checkAndLogMsg ("Mocket::resumeAndRestoreState", Logger::L_SevereError,
                    "Crypto is disabled\n");
    return -1;
#else
    // Set the start time
    uint64 ui64StartTime = getTimeInMilliseconds();
    
    // Set the state in S_SUSPENDED
    if (! _sm.resumeFromSuspension ()) {
        return -1;
    }
    
    int rc;
    
    ObjectDefroster objectDefroster;
    objectDefroster.init (pr);
    
    // Extract Mocket variables to start the resume process
    objectDefroster.beginNewObject ("Mocket");
    // Variables in Mocket
    objectDefroster.beginNewObject ("MocketVariables");
    char * pszIdentifier = NULL;
    objectDefroster >> pszIdentifier;
    setIdentifier (pszIdentifier);
    delete[] pszIdentifier;
    pszIdentifier = NULL;
    objectDefroster >> _bEnableCrossSequencing;
    objectDefroster >> _bOriginator;
    objectDefroster >> _ui32RemoteAddress;
    objectDefroster >> _ui16RemotePort;
    objectDefroster >> _ui32LingerTime;
    objectDefroster >> _bEnableXMitLogging;
    objectDefroster >> _bUseBandwidthEstimation;

//TODO    objectDefroster >> _pszCongestionControl;
    objectDefroster >> _pszPassword;
    objectDefroster >> _ui32MocketUUID;
    uint32 ui32OutgoingValidation;
    objectDefroster >> ui32OutgoingValidation;
    if (0 != (rc = objectDefroster.endObject())) {
        return -2;
    }
 
    // Check the parameter ResumeTimeout
    if (ui32ResumeTimeout < DEFAULT_MIN_RESUME_TIMEOUT) {
        //The time to wait in RESUME_SEND state is too short
        ui32ResumeTimeout = DEFAULT_MIN_RESUME_TIMEOUT;
    }
    
    // _bMocketAlreadyBound is not carried over after migration, its value is true only if
    // a new bind was performed before the call to resumeAndRestoreState
    if (!_bMocketAlreadyBound) {
        InetAddr bindAddr;
        if (0 != (rc = _pCommInterface->bind (&bindAddr))) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "could not initialize DatagramSocket; rc = %d\n", rc);
            return -3;
        }
    }

    _ui32LocalAddress = _pCommInterface->getLocalAddr().getIPAddress();
    _ui16LocalPort = _pCommInterface->getLocalPort();
    //printf ("Mocket::resumeAndRestoreState * LOCAL during resume ip %lu * port %d *\n", _ui32LocalAddress, _ui16LocalPort);

    if (0 != (rc = _pCommInterface->setReceiveBufferSize (_ui16UDPBufferSize))) {
        checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                        "could not set UDP receive buffer size to %d; rc = %d\n", (int) _ui16UDPBufferSize, rc);
        return -4;
    }
    if (0 != (rc = _pCommInterface->setReceiveTimeout (_ui32UDPReceiveConnectionTimeout))) {
        checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                        "could not set UDP timeout to %d; rc = %d\n", _ui32UDPReceiveConnectionTimeout, rc);
        return -5;
    }
    
    // Create the resume packet
    Packet resumePacket (this);
    resumePacket.setValidation (ui32OutgoingValidation);

    // Encrypt nonce (_ui32MocketUUID), local IP address and port with Ks
    char pchBuff[10]; // 4 dimension of _ui32MocketUUID + 4 dimension of _ui32LocalAddress + 2 dimension of _ui16LocalPort
    uint32 ui32BuffSize = 0;
    // Insert the UUID
    *((uint32*)(pchBuff)) = getMocketUUID();
    ui32BuffSize += 4;
    //TODO the local IP is 0!!
    // Insert the local IP address
    *((uint32*)(pchBuff+ui32BuffSize)) = _ui32LocalAddress;
    ui32BuffSize += 4;
    // Insert the local port
    *((uint16*)(pchBuff+ui32BuffSize)) = _ui16LocalPort;
    ui32BuffSize += 2;

    // Reconstruct the secret key from the password
    newSecretKey (_pszPassword);
    // Set that UUID and Ks are valid so reEstablishConn is supported
    setSupportReEstablish (true);
    // Encrypt
    uint32 ui32EncryptedDataLen = 0;
    void *pEncryptedData = encryptDataUsingSecretKey (_pSecretKey, pchBuff, ui32BuffSize, &ui32EncryptedDataLen);
    if (pEncryptedData == NULL) {
        checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError, 
                         "unable to encrypt nonce with secret key\n");
        return -6;
    }
    if (resumePacket.addResumeChunk(pEncryptedData, ui32EncryptedDataLen)) {
        return -7;
    }
    
    // Send resume and wait for resume_ack
    uint64 ui64ResumeSent = 0;
    char achReplyBuf [MAXIMUM_MTU];
    while (true) {
        InetAddr remoteAddr;
        int rc;
        if ((getTimeInMilliseconds() - ui64ResumeSent) > getMaxSendNewSuspendResumeTimeout()) {
            // Send resume
            //printf ("Mocket::resumeAndRestoreState Send resume\n");
            InetAddr remoteAddr (_ui32RemoteAddress, _ui16RemotePort);
            rc = _pCommInterface->sendTo (&remoteAddr, resumePacket.getPacket(), resumePacket.getPacketSize());
            if (rc < 0) {
                checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                                "send on datagram socket failed with rc = %d; os error = %d\n",
                                rc, _pCommInterface->getLastError());
            }
            else {
                // Change the state in S_RESUME_SENT
                if (! _sm.resume()) {
                    return -8;
                }
                ui64ResumeSent = getTimeInMilliseconds();
            }
        }
        // Wait for resume_ack
        //printf ("Mocket::resumeAndRestoreState Wait for resume_ack\n");
        rc = _pCommInterface->receive (achReplyBuf, MAXIMUM_MTU, &remoteAddr);
        if (rc < 0) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "receive on datagram socket failed with rc = %d; os error = %d\n",
                            rc, _pCommInterface->getLastError());
        }
        else if (rc == 0) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "timed out waiting for reply on Resume packet from: %s:%d\n", 
                            (const char*) remoteAddr.getIPAsString(), remoteAddr.getPort());
        }
        else if (rc <= Packet::HEADER_SIZE) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "received a short packet; rc = %d\n", rc);
        }
        else if ((remoteAddr.getIPAddress() != _ui32RemoteAddress) || (remoteAddr.getPort() != _ui16RemotePort)) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "received a packet from a different endpoint %s:%d\n",
                            (char*)remoteAddr.getIPAsString(), remoteAddr.getPort());
        }
        else {
            // Parse the packet and look for the ResumeAck chunk
            Packet replyPacket (achReplyBuf, rc);
            if (replyPacket.getChunkType() != Packet::CT_ResumeAck) {
                checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                                "received a reply packet without the ResumeAck chunk (type %d) as the first chunk; chunk type is %d\n",
                                (int) Packet::CT_ResumeAck, (int) replyPacket.getChunkType());
            }
            else {
                // Found the ResumeAck chunk go in ESTABLISHED state
                if (!_sm.receivedResumeAck()) {
                    checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                                    "mocket state machine in illegal state\n");
                    _sm.abort();
                    return -9;
                }
                break;
            }
        }
        // Check if the timeout is expired
        if ((uint64) getTimeInMilliseconds() > (ui64StartTime + (uint64) ui32ResumeTimeout)) {
            checkAndLogMsg ("Mocket::resumeFromSuspension", Logger::L_MildError,
                            "resume timeout expired while waiting for ResumeAck\n");
            _sm.abort();
            InetAddr localAddr;
            if (_pszLocalAddress != NULL) {
                localAddr.setIPAddress (_pszLocalAddress);
            }
            else {
                localAddr = _pCommInterface->getLocalAddr();
            }
            InetAddr connectToAddr (_ui32RemoteAddress);
            _pMocketStatusNotifier->connectionFailed (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &connectToAddr, _ui16RemotePort);
            return -10;
        }
    }
    
    // Initialize the Mocket extracting the other frozen objects from objectDefroster
    if (0 != defrost (objectDefroster)) {
        return -11;
    }
    //End of the Mocket frozen object
    if (objectDefroster.endObject()) {
        return -12;
    }
    
    // Connection restored - Call MocketStatusNotifier
    InetAddr localAddr;
    if (_pszLocalAddress != NULL) {
        localAddr.setIPAddress (_pszLocalAddress);
    }
    else {
        localAddr = _pCommInterface->getLocalAddr();
    }
    InetAddr remoteAddr (_ui32RemoteAddress);
    _pMocketStatusNotifier->connectionRestored (getIdentifier(), &localAddr, _pCommInterface->getLocalPort(), &remoteAddr, _ui16RemotePort);
    
    _pPacketProcessor->reinitAfterDefrost();
    startThreads();

    return 0;
#endif
}

int Mocket::close (void)
{
    StateMachine::State state = _sm.getCurrentState();
    if (state == StateMachine::S_CLOSED) {
        return 0;
    }
    else if (state == StateMachine::S_ESTABLISHED) {
        _sm.shutdown();
        _pTransmitter->setShutdownStartTime();
        _pTransmitter->notify();
        return 0;
    }
    else if (state == StateMachine::S_SUSPEND_RECEIVED) {
        checkAndLogMsg ("Mocket::close", Logger::L_MildError,
                        "current state is SUSPEND_RECEIVED, abort the connection\n");
        // Abort the connection without the shutdown because there is no communication with the other side
        _sm.abort();
        return 0;
    }
    return -1;
}

MessageSender Mocket::getSender (bool bReliable, bool bSequenced)
{
    return MessageSender (this, bReliable, bSequenced);
}

uint32 Mocket::getOutgoingBufferSize (void)
{
    if (_pTransmitter) {
        return _pTransmitter->getSpaceAvailable();
    }
    else {
        return 0;
    }
}

int Mocket::send (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16Tag, uint8 ui8Priority,
                  uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    /*!!*/ // Need to unblock the calling thread if the mocket is closed (or if the other end closes the mocket)
    /*!!*/ // Address race condition with deleting the Mocket while another thread is trying to send
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->send (bReliable, bSequenced, pBuf, ui32BufSize, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout);
}

int Mocket::gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                   const void *pBuf1, uint32 ui32BufSize1, ...)
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    int rc;
    va_list valist1, valist2;
    va_start (valist1, ui32BufSize1);
    va_start (valist2, ui32BufSize1);
    rc = gsend (bReliable, bSequenced, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout, pBuf1, ui32BufSize1, valist1, valist2);
    va_end (valist1);
    va_end (valist2);
    return rc;
}

int Mocket::gsend (bool bReliable, bool bSequenced, uint16 ui16Tag, uint8 ui8Priority, uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout,
                   const void *pBuf1, uint32 ui32BufSize1, va_list valist1, va_list valist2)
{
    /*!!*/ // See comments in the send() above
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->gsend (bReliable, bSequenced, ui16Tag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout, pBuf1, ui32BufSize1, valist1, valist2);
}

int Mocket::getNextMessageSize (int64 i64Timeout)
{
    /*!!*/ // Investigate comment below with receive
    if (_pPacketProcessor == NULL) {
        return -1;
    }
    return _pPacketProcessor->getNextMessageSize (i64Timeout);
}

uint32 Mocket::getCumulativeSizeOfAvailableMessages (void)
{
    if (_pPacketProcessor == NULL) {
        return 0;
    }
    return _pPacketProcessor->getCumulativeSizeOfAvailableMessages();
}

int Mocket::receive (void *pBuf, uint32 ui32BufSize, int64 i64Timeout)
{
    /*!!*/ // Need to unblock the calling thread if the mocket is closed (or if the other end closes the mocket)
    /*!!*/ // Address race condition with deleting the Mocket while another thread is trying to receive
    if (_pPacketProcessor == NULL) {
        return -1;
    }
    return _pPacketProcessor->receive (pBuf, ui32BufSize, i64Timeout);
}

int Mocket::receive (void **ppBuf, int64 i64Timeout)
{
    int iMsgSize = getNextMessageSize (i64Timeout);
    if (iMsgSize <= 0) {
        return iMsgSize;
    }
    void *pBuf = malloc (iMsgSize);
    int rc = receive (pBuf, iMsgSize, i64Timeout);
    if (rc <= 0) {
        (*ppBuf) = NULL;
        free (pBuf);
        return rc;
    }
    else {
        (*ppBuf) = pBuf;
        return rc;
    }
}

int Mocket::sreceive (int64 i64Timeout, void *pBuf1, uint32 ui32BufSize1, ...)
{
    if (_pPacketProcessor == NULL) {
        return -1;
    }
    va_list valist;
    va_start (valist, ui32BufSize1);
    int rc = _pPacketProcessor->sreceive (i64Timeout, pBuf1, ui32BufSize1, valist);
    va_end (valist);
    return rc;
}

int Mocket::replace (bool bReliable, bool bSequenced, const void *pBuf, uint32 ui32BufSize, uint16 ui16OldTag, uint16 ui16NewTag, uint8 ui8Priority,
                     uint32 ui32EnqueueTimeout, uint32 ui32RetryTimeout)
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    
    uint8 ui8HigherPriority = 0;
    
    if (_pTransmitter->cancel (bReliable, bSequenced, ui16OldTag, &ui8HigherPriority) < 0) {
        return -2;
    }
    // Preserve the priority of the message being replaced if it is higher than the one of the new msg
    if (ui8HigherPriority > ui8Priority) {
        ui8Priority = ui8HigherPriority;
    }
    
    /*!!*/ // See if the comment in send() above applies here also
    if (_pTransmitter->send (bReliable, bSequenced, pBuf, ui32BufSize, ui16NewTag, ui8Priority, ui32EnqueueTimeout, ui32RetryTimeout) < 0) {
        return -3;
    }
    return 0;
}

int Mocket::cancel (bool bReliable, bool bSequenced, uint16 ui16TagId)
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->cancel (bReliable, bSequenced, ui16TagId);
}

int Mocket::readConfigFile (const char *pszConfigFile)
{
    checkAndLogMsg ("Mocket::readConfigFile", Logger::L_Info, "loading configuration from file: %s\n", pszConfigFile);
    return initParamsFromConfigFile (pszConfigFile);
}

void Mocket::resetTransmissionCounters (void)
{
    if (_pTransmitter == NULL) {
        return;
    }
    // Reset the estimated value of the smoothed RTT for this connection
    _pTransmitter->resetSRTT();
    // Reset retransmit count and retransmit timeouts of the packets in pending packet queues
    _pTransmitter->resetUnackPacketsRetransmitTimeoutRetransmitCount (_pTransmitter->getRetransmissionTimeout());
    checkAndLogMsg ("Mocket::resetTransmissionCounters", Logger::L_Info, "RTT, unack packets retr count and retr timeout\n");
}

void Mocket::closed (void)
{
    // NOTE: There might be another thread that is currently blocked in the Mocket destructor, blocked
    // on the TermSync::waitForComponentTermination() method, which will be unblocked as soon as this method returns
    _pMocketStatusNotifier->sendStats (getIdentifier(), getStatistics());
    _pMocketStatusNotifier->disconnected (getIdentifier());
}

//Must be called after connect()
int Mocket::activateBandwidthEstimation (uint16 ui16InitialAssumedBandwidth)
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->setBandwidthEstimationActive (ui16InitialAssumedBandwidth);
}

//Must be called after connect()
int Mocket::activateCongestionControl()
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->setCongestionControlActive (_pszCongestionControl);
}

int Mocket::setTransmitRateLimit (uint32 ui32TransmitRateLimit)
{
    if (_pTransmitter == NULL) {
        return -1;
    }
    return _pTransmitter->setTransmitRateLimit (ui32TransmitRateLimit);
}

int Mocket::suspend (uint32 ui32FlushDataTimeout, uint32 ui32SuspendTimeout)
{
#ifdef MOCKETS_NO_CRYPTO
    checkAndLogMsg ("Mocket::suspend", Logger::L_SevereError,
                    "Crypto is disabled\n");
    return -1;
#else
    _mSuspend.lock();
    if (_sm.getCurrentState() == StateMachine::S_SUSPEND_RECEIVED) {
        // Already suspended due to suspend message from the other side of the mocket
        checkAndLogMsg ("Mocket::suspend", Logger::L_MildError,
                        "already suspended due to suspend message from the other side of the mocket\n");
        _mSuspend.unlock();
        return -1;
    }
    else if(_sm.getCurrentState() != StateMachine::S_ESTABLISHED) {
        // Not in a consistent state for start a suspension
        checkAndLogMsg ("Mocket::suspend", Logger::L_MildError,
                        "not in a consistent state for start a suspension\n");
        _mSuspend.unlock();
        return -2;
    }
    // Check the parameter SuspendTimeout
    // We allow 0 for FlushDataTimeout, 0 means that application doesn't want to flush data
    if (ui32SuspendTimeout < DEFAULT_MIN_SUSPEND_TIMEOUT) {
        //The time to wait in SUSPEND_SEND state is too short
        ui32SuspendTimeout = DEFAULT_MIN_SUSPEND_TIMEOUT;
    }
    
    if (!areKeysExchanged()) {
        // Create a new pair of keys
        _pKeyPair = new PublicKeyPair();
        if (0 != _pKeyPair->generateNewKeyPair()) {
            // Error while creating publicKeyPair
            checkAndLogMsg ("Mocket::suspend", Logger::L_MildError,
                            "error while creating publicKeyPair\n");
            _mSuspend.unlock();
            return -3;
        }
    }
    // Initialize timeouts
    _ui32FlushDataTimeout = ui32FlushDataTimeout;
    _ui32SuspendTimeout = ui32SuspendTimeout;
    // Flush data
    if (!_pTransmitter->waitForFlush()) {
        checkAndLogMsg ("Mocket::suspend", Logger::L_MildError,
                        "error while waiting for flushing data\n");
        // If wait for flush does not work we don't have to change the state
        _mSuspend.unlock();
        return -4;
    }
    // Send suspend and wait for suspend_ack
    if (!_pTransmitter->suspend()) {
        checkAndLogMsg ("Mocket::suspend", Logger::L_MildError,
                        "error while suspending\n");
        // If the current state is suspend_sent we need to change it otherwise we will continue to send suspend msg.
        // We can simply set the state to ESTABLISHED and so the application can try to send msg or to close 
        // (eventually the close will cause an abort).
        // This method change the state to established only if the current state is suspend_sent
        _sm.suspendTimeoutExpired();

        _mSuspend.unlock();
        return -5;
    }
    // Suspension successfully finished
    _mSuspend.unlock();
    return 0;
#endif
}

int Mocket::getState (Writer *pw)
{
    if (_sm.getCurrentState() != StateMachine::S_SUSPENDED) {
        return -1;
    }
    
    // Create ObjectFreezer
    ObjectFreezer objectFreezer;
    objectFreezer.init (pw);
    
    // Call Mocket::freeze, this will create a super object containing all the state variables
    int rc;
    if (0 != (rc = freeze (objectFreezer))) {
        return rc;
    }
    return 0;
}

int Mocket::resolveSimultaneousSuspension (void)
{
    if (getOutgoingValidation() > getIncomingValidation()) {
        return 0;
    }
    else if (getOutgoingValidation() == getIncomingValidation()) {
        if (_ui32LocalAddress > _ui32RemoteAddress) {
            return 0;
        }
        else if (_ui32LocalAddress == _ui32RemoteAddress) {
            if (_ui16LocalPort > _ui16RemotePort) {
                return 0;
            }
            else if (_ui16LocalPort == _ui16RemotePort) {
                // No way to resolve the conflict
                return -1;
            }
        }
    }
    return 1;
}

void Mocket::newSecretKey (char *pszPassword)
{
#ifdef MOCKETS_NO_CRYPTO
    checkAndLogMsg ("Mocket::newSecretKey", Logger::L_SevereError,
                "crypto disabled at build time\n");

#else
    _pSecretKey = new SecretKey();
    if (0 != _pSecretKey->initKey(pszPassword)) {
        // Error while creating secretKey
        checkAndLogMsg ("Mocket::newSecretKey", Logger::L_MildError,
                        "error while creating secretKey\n");
    }
#endif
}

int Mocket::freeze (ObjectFreezer &objectFreezer)
{
    objectFreezer.beginNewObject ("Mocket");
    objectFreezer.beginNewObject ("MocketVariables");
    objectFreezer.putString (_identifier);
    // TODO:
    //_pPeerUnreachableWarningCallbackFn how?
    //_pCallbackArg how?
    //_pSuspendReceivedWarningCallbackFn how?
    //_pSuspendReceivedCallbackArg how?
    
    // do not freeze _sm
    objectFreezer.putBool (_bEnableCrossSequencing);
    objectFreezer.putBool (_bOriginator);
    // do not freeze _ui16UDPBufferSize initialized in the constructor with a default value and not changed
    objectFreezer.putUInt32 (_ui32RemoteAddress);
    objectFreezer.putUInt16 (_ui16RemotePort);
    // do not freeze _ui32LocalAddress _ui16LocalPort
    // do not freeze _pDGSocket
    // do not freeze MocketStatusNotifier *_pMocketStatusNotifier;
    // do not freeze _ui32UDPReceiveTimeout _ui16KeepAliveTimeout _ui16InitialAssumedRTT  _ui16MinimumRTT
    // _ui32MaximumWindowSize _ui16SAckTransmitTimeout are initialized with a default
    //      values and not changed also initialized from value in the config file
    objectFreezer.putUInt32 (_ui32LingerTime);
    objectFreezer.putBool (_bEnableXMitLogging);
    objectFreezer.putBool (_bUseBandwidthEstimation);
//    objectFreezer.putString (_pszCongestionControl);
    // do not freeze _ui32SuspendTimeout _ui32FlushDataTimeout set every new suspend/resume call
    // do not freeze _pKeyPair created new with every suspend
    // do not freeze _pSecretKey created from the password
    objectFreezer.putString (_pszPassword);
    objectFreezer.putUInt32 (_ui32MocketUUID);
    // do not freeze _bKeysExchanged, to perform this action this variable must be true
    // Outgoing validation comes from StateCookie but we need it to restore the connection
    objectFreezer.putUInt32(getOutgoingValidation());
    objectFreezer.endObject();
    
    // StateCookie
    if (0 != _stateCookie.freeze (objectFreezer)) {
        // return -1 is if objectFreezer.endObject() don't end with success
        return -2;
    }
    // MocketStats
    if (0 != _stats.freeze (objectFreezer)) {
        return -3;
    }
    // PacketProcessor
    // This must be before Receiver. Then calling reinitAfterDefrost() we copy the reference to the queue
    if (0 != _pPacketProcessor->freeze (objectFreezer)) {
        return -4;
    }
    // Receiver
    if (0 != _pReceiver->freeze (objectFreezer)) {
        return -5;
    }
    // Transmitter
    if (0 != _pTransmitter->freeze (objectFreezer)) {
        return -6;
    }
    // ACKManager
    if (0 != _ackManager.freeze (objectFreezer)) {
        return -7;
    }
    // CancelledTSNManager
    if (0 != _cancelledTSNManager.freeze (objectFreezer)) {
        return -8;
    }
    
    return objectFreezer.endObject();
}

int Mocket::defrost (ObjectDefroster &objectDefroster)
{
    // StateCookie
    if (0 != _stateCookie.defrost (objectDefroster)) {
        return -2;
    }
    // MocketStats
    if (0 != getStatistics()->defrost (objectDefroster)) {
        return -3;
    }
    // PacketProcessor
    _pPacketProcessor = new PacketProcessor (this);
    if (0 != _pPacketProcessor->defrost (objectDefroster)) {
        return -4;
    }
    // Receiver
    _pReceiver = new Receiver (this, _bEnableRecvLogging);
    if (0 != _pReceiver->defrost (objectDefroster)) {
        return -5;
    }
    // Transmitter
    _pTransmitter = new Transmitter (this, _bEnableXMitLogging);
    if (0 != _pTransmitter->defrost (objectDefroster)) {
        return -6;
    }    
    // ACKManager
    if (0 != _ackManager.defrost (objectDefroster)) {
        return -7;
    }
    // CancelledTSNManager
    if (0 != _cancelledTSNManager.defrost (objectDefroster)) {
        return -8;
    }
    
    return objectDefroster.endObject();
}

void Mocket::newMocketUUID (void)
{
    srand ((uint32)getTimeInMilliseconds());
    uint16 ui16RandomNumber1 = rand();
    uint16 ui16RandomNumber2 = rand();
    _ui32MocketUUID = (uint32) ((ui16RandomNumber1 << 16) | ui16RandomNumber2);
    //printf ("Mocket::newMocketUUID UUID = [%lu]\n", _ui32MocketUUID);
}

void Mocket::newSecretKey (void)
{
#ifdef MOCKETS_NO_CRYPTO
    checkAndLogMsg ("Mocket::newSecretKey", Logger::L_SevereError,
                "crypto disabled at build time\n");
#else
    _pSecretKey = new SecretKey();
    // Create a new random password
    srand ((uint32)getTimeInMilliseconds());
    uint16 passwordLength = DEFAULT_PASSWORD_LENGTH;
    _pszPassword = new char [passwordLength + 1];
    _pszPassword[passwordLength] = 0;
    
    for (int i = 0; i < passwordLength; i++) {
        // generate a random value between 'a' and 'z', 'A' and 'Z', 0 and 9
        _pszPassword[i] = (char) (rand() % ('z' - '0' + 1) + '0');
    }
    //printf ("Mocket::newSecretKey PASSWORD = [%s]\n", _pszPassword);
    
    // Create a new secret key from the password
    if (0 != _pSecretKey->initKey(_pszPassword)) {
        // Error while creating secretKey
        checkAndLogMsg ("Mocket::newSecretKey", Logger::L_MildError,
                        "error while creating secretKey\n");
    }    
#endif
}

void Mocket::setPassword (const char *pszPassword)
{
    uint16 ui16PassLen = (uint16)strlen(pszPassword);
    if (_pszPassword) {
        delete [] _pszPassword;
        _pszPassword = NULL;
    }
    _pszPassword = new char [ui16PassLen+1];
    memcpy (_pszPassword, pszPassword, strlen(pszPassword)+1);
}

void Mocket::notifyTransmitter (void)
{
    _pTransmitter->notify();
}

void Mocket::notifyPacketProcessor (void)
{
    _pPacketProcessor->packetArrived();
}

Mocket::TermSync::TermSync (Mocket *pMocket)
    : _cv (&_m)
{
    _pMocket = pMocket;
    _bPacketProcessorTerminated = false;
    _bReceiverTerminated = false;
    _bTransmitterTerminated = false;
}

void Mocket::TermSync::packetProcessorTerminating (void)
{
    _m.lock();
    _bPacketProcessorTerminated = true;
    checkAndLogMsg ("Mocket::TermSync::packetProcessorTerminating", Logger::L_MediumDetailDebug,
            "packetProcessor terminated, notify\n");
    checkIfAllTerminated();  // Must be called before unlocking _m
    _cv.notifyAll();
    _m.unlock();
}

void Mocket::TermSync::receiverTerminating (void)
{
    _m.lock();
    _bReceiverTerminated = true;
    checkAndLogMsg ("Mocket::TermSync::receiverTerminating", Logger::L_MediumDetailDebug,
            "receiver terminated, notify\n");
    checkIfAllTerminated();  // Must be called before unlocking _m
    _cv.notifyAll();
    _m.unlock();
}

void Mocket::TermSync::transmitterTerminating (void)
{
    _m.lock();
    _bTransmitterTerminated = true;
    checkAndLogMsg ("Mocket::TermSync::transmitterTerminating", Logger::L_MediumDetailDebug,
            "transmitter terminated, notify\n");
    checkIfAllTerminated();  // Must be called before unlocking _m
    _cv.notifyAll();
    _m.unlock();
}

void Mocket::TermSync::waitForComponentTermination (void)
{
    _m.lock();
    while ((!_bPacketProcessorTerminated) || (!_bReceiverTerminated) || (!_bTransmitterTerminated)) {
        _cv.wait();
        checkAndLogMsg ("Mocket::TermSync::waitForComponentTermination", Logger::L_MediumDetailDebug,
                "woken up, check if threads terminated\n");
    }
    checkAndLogMsg ("Mocket::TermSync::waitForComponentTermination", Logger::L_MediumDetailDebug,
            "threads terminated\n");
    _m.unlock();
}

void Mocket::TermSync::checkIfAllTerminated (void)
{
    // Check to see if all three components have terminated
    // If so - invoke a callback on the Mocket class
    // NOTE: The Mocket may be in its destructor - blocked on the above call to waitForComponentTermination()
    if ((_bPacketProcessorTerminated) && (_bReceiverTerminated) && (_bTransmitterTerminated)) {
        _pMocket->closed();
    }
}

Mocket::AsynchronousConnector::AsynchronousConnector (Mocket *pMocket, const char *pszRemoteHost, uint16 ui16RemotePort)
{
    _pMocket = pMocket;
    _pszHost = strDup (pszRemoteHost);
    _ui16Port = ui16RemotePort;
    _bIsRunning = false;
    _iConnectionRes = 10;
}

void Mocket::AsynchronousConnector::run (void)
{
    _bIsRunning = true;
    // Asynchronously connect the mocket
    _iConnectionRes = _pMocket->connect(_pszHost, _ui16Port);
    _bIsRunning = false;
}

bool Mocket::AsynchronousConnector::isRunning (void)
{
    return _bIsRunning;
}

int Mocket::AsynchronousConnector::connectionRes (void)
{
    return _iConnectionRes;
}

void Mocket::enableTransmitLogging (bool bEnableXMitLogging)
{
    _bEnableXMitLogging = bEnableXMitLogging;
    if (_pTransmitter) {
        _pTransmitter->enableTransmitLogging (bEnableXMitLogging);
    }
}
