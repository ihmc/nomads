#ifndef INCL_PROTOCOL_SETTING_H
#define INCL_PROTOCOL_SETTING_H

/*
 * ProtocolSetting.h
 *
 * This file is part of the IHMC NetProxy Library/Component
 * Copyright (c) 2010-2016 IHMC.
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
 *
 * Class that stores the protocol and the compression
 * used to send a message to a remote NetProxy.
 */

#include "ProxyMessages.h"
#include "CompressionSetting.h"


namespace ACMNetProxy
{
    class ProtocolSetting
    {
    public:
        explicit ProtocolSetting (const char * const pProtocol);
        explicit ProtocolSetting (const ProxyMessage::Protocol protocolFlag);
        ProtocolSetting (const char * const pProtocol, const CompressionSetting &compressionSetting);
        ProtocolSetting (const ProxyMessage::Protocol protocolFlag, const CompressionSetting &compressionSetting);
        ProtocolSetting (const ProtocolSetting &protocolSetting);
        ~ProtocolSetting (void);

        ProxyMessage::Protocol getProxyMessageProtocol (void) const;
        const char * const getProxyMessageProtocolAsString (void) const;
        static const char * const getProxyMessageProtocolAsString (ProxyMessage::Protocol protocol);
        const uint8 getPriorityLevel (void) const;
        const EncryptionType getEncryptionType (void) const;
        const CompressionSetting & getCompressionSetting (void) const;

        const bool isDefinedProtocol (void) const;
        const bool isMocketsProtocol (void) const;
        const bool isTCPProtocol (void) const;
        const bool isUDPProtocol (void) const;
        const bool isSameProtocolFamily (const ProxyMessage::Protocol protocol) const;
        const bool isSameProtocolFamily (const ConnectorType connectorType) const;
        ConnectorType getConnectorTypeFromProtocol (void) const;

        void setProxyMessageProtocol (const ProxyMessage::Protocol protocolFlag);
        void setPriorityLevel (const uint8 priorityLevel);
        void setEncryption (const EncryptionType encryptionType);
        void setCompressionSetting (const CompressionSetting compressionSetting);

        static const bool isDefinedProtocol (const ProxyMessage::Protocol protocol);
        static const bool isMocketsProtocol (const ProxyMessage::Protocol protocol);
        static const bool isTCPProtocol (const ProxyMessage::Protocol protocol);
        static const bool isUDPProtocol (const ProxyMessage::Protocol protocol);
        static const bool isCSRProtocol (const ProxyMessage::Protocol protocol);
        static const bool isSameProtocolFamily (const ProxyMessage::Protocol protocolA, const ProxyMessage::Protocol protocolB);
        static const bool isSameProtocolFamily (const ProxyMessage::Protocol protocol, const ConnectorType connectorType);
        static bool isProtocolNameCorrect (const char * const pProtocolToCheck);

        static const ProtocolSetting & getInvalidProtocolSetting (void);
        static const ProtocolSetting * const getDefaultICMPProtocolSetting (void);
        static const ProtocolSetting * const getDefaultTCPProtocolSetting (void);
        static const ProtocolSetting * const getDefaultUDPProtocolSetting (void);

        static ProxyMessage::Protocol getProtocolFlagFromProtocolString (const char * const pProtocolName);
        static ConnectorType protocolToConnectorType (const ProxyMessage::Protocol protocol);
        static ProxyMessage::Protocol connectorTypeToProtocol (const ConnectorType connectorType);


    private:
        static const ProtocolSetting INVALID_PROTOCOL_SETTING;
        static const ProtocolSetting DEFAULT_TCP_PROTOCOL_SETTING;
        static const ProtocolSetting DEFAULT_UDP_PROTOCOL_SETTING;
        static const ProtocolSetting DEFAULT_ICMP_PROTOCOL_SETTING;

        ProxyMessage::Protocol _protocol;
        uint8 _ui8Priority;
        EncryptionType _encryptionType;
        CompressionSetting _compressionSetting;
    };

    inline ProtocolSetting::ProtocolSetting (const char * const pProtocol) :
		_protocol(getProtocolFlagFromProtocolString (pProtocol)), _ui8Priority(0), _encryptionType(ET_PLAIN), _compressionSetting() {}

    inline ProtocolSetting::ProtocolSetting (const ProxyMessage::Protocol protocolFlag) :
		_protocol(protocolFlag), _ui8Priority(0), _encryptionType(ET_PLAIN), _compressionSetting() { }

    inline ProtocolSetting::ProtocolSetting (const char * const pProtocol, const CompressionSetting &compressionSetting) :
		_protocol(getProtocolFlagFromProtocolString (pProtocol)), _ui8Priority(0), _encryptionType(ET_PLAIN), _compressionSetting(compressionSetting) { }

    inline ProtocolSetting::ProtocolSetting (const ProxyMessage::Protocol protocolFlag, const CompressionSetting &compressionSetting) :
		_protocol(protocolFlag), _ui8Priority(0), _encryptionType(ET_PLAIN), _compressionSetting(compressionSetting) { }

    inline ProtocolSetting::ProtocolSetting (const ProtocolSetting &protocolSetting) :
		_protocol(protocolSetting._protocol), _ui8Priority(0), _encryptionType(ET_PLAIN), _compressionSetting(protocolSetting._compressionSetting) { }

    inline ProtocolSetting::~ProtocolSetting (void) {}

    inline ProxyMessage::Protocol ProtocolSetting::getProxyMessageProtocol (void) const
    {
        return _protocol;
    }

    inline const char * const ProtocolSetting::getProxyMessageProtocolAsString (void) const
    {
        return ProtocolSetting::getProxyMessageProtocolAsString (_protocol);
    }

    inline const uint8 ProtocolSetting::getPriorityLevel (void) const
    {
        return  _ui8Priority;
    }

    inline const EncryptionType ProtocolSetting::getEncryptionType (void) const
    {
        return _encryptionType;
    }

    inline const CompressionSetting & ProtocolSetting::getCompressionSetting (void) const
    {
        return _compressionSetting;
    }

    inline const bool ProtocolSetting::isDefinedProtocol (void) const
    {
        return ProtocolSetting::isDefinedProtocol (_protocol);
    }

    inline const bool ProtocolSetting::isMocketsProtocol (void) const
    {
        return ProtocolSetting::isMocketsProtocol (_protocol);
    }

    inline const bool ProtocolSetting::isTCPProtocol (void) const
    {
        return ProtocolSetting::isTCPProtocol (_protocol);
    }

    inline const bool ProtocolSetting::isUDPProtocol (void) const
    {
        return ProtocolSetting::isUDPProtocol (_protocol);
    }

    inline const bool ProtocolSetting::isSameProtocolFamily (const ProxyMessage::Protocol protocol) const
    {
        return ProtocolSetting::isSameProtocolFamily (_protocol, protocol);
    }

    inline const bool ProtocolSetting::isSameProtocolFamily (const ConnectorType connectorType) const
    {
        return ProtocolSetting::isSameProtocolFamily (_protocol, connectorType);
    }

    inline ConnectorType ProtocolSetting::getConnectorTypeFromProtocol (void) const
    {
        return protocolToConnectorType (_protocol);
    }

    inline void ProtocolSetting::setProxyMessageProtocol (const ProxyMessage::Protocol protocolFlag)
    {
        _protocol = protocolFlag;
    }

    inline void ProtocolSetting::setPriorityLevel (const uint8 priorityLevel)
    {
        _ui8Priority = priorityLevel;
    }

    inline void ProtocolSetting::setEncryption (const EncryptionType encryptionType)
    {
        _encryptionType = encryptionType;
    }

    inline void ProtocolSetting::setCompressionSetting (const CompressionSetting compressionSetting)
    {
        _compressionSetting = compressionSetting;
    }

    inline const bool ProtocolSetting::isDefinedProtocol (const ProxyMessage::Protocol protocol)
    {
        return (protocol >= ProxyMessage::PMP_TCP) && (protocol <= ProxyMessage::PMP_UNDEF_CSR);
    }

    inline const bool ProtocolSetting::isMocketsProtocol (const ProxyMessage::Protocol protocol)
    {
        return (protocol >= ProxyMessage::PMP_MocketsRS) && (protocol <= ProxyMessage::PMP_UNDEF_MOCKETS);
    }

    inline const bool ProtocolSetting::isTCPProtocol (const ProxyMessage::Protocol protocol)
    {
        return protocol == ProxyMessage::PMP_TCP;
    }

    inline const bool ProtocolSetting::isUDPProtocol (const ProxyMessage::Protocol protocol)
    {
        return protocol == ProxyMessage::PMP_UDP;
    }

    inline const bool ProtocolSetting::isCSRProtocol (const ProxyMessage::Protocol protocol)
    {
        return (protocol >= ProxyMessage::PMP_CSRRS) && (protocol <= ProxyMessage::PMP_UNDEF_CSR);
    }

    inline const bool ProtocolSetting::isSameProtocolFamily (const ProxyMessage::Protocol protocolA, const ProxyMessage::Protocol protocolB)
    {
        return (isMocketsProtocol (protocolA) && isMocketsProtocol (protocolB)) ||
               (isTCPProtocol (protocolA) && isTCPProtocol (protocolB)) ||
               (isUDPProtocol (protocolA) && isUDPProtocol (protocolB));
    }

    inline const bool ProtocolSetting::isSameProtocolFamily (const ProxyMessage::Protocol protocol, const ConnectorType connectorType)
    {
        return (isMocketsProtocol (protocol) && (connectorType == CT_MOCKETS)) ||
               (isTCPProtocol (protocol) && (connectorType == CT_TCPSOCKET)) ||
               (isUDPProtocol (protocol) && (connectorType == CT_UDPSOCKET));
    }

    inline const ProtocolSetting & ProtocolSetting::getInvalidProtocolSetting (void)
    {
        return INVALID_PROTOCOL_SETTING;
    }

    inline const ProtocolSetting * const ProtocolSetting::getDefaultTCPProtocolSetting (void)
    {
        return &DEFAULT_TCP_PROTOCOL_SETTING;
    }

    inline const ProtocolSetting * const ProtocolSetting::getDefaultUDPProtocolSetting (void)
    {
        return &DEFAULT_UDP_PROTOCOL_SETTING;
    }

    inline const ProtocolSetting * const ProtocolSetting::getDefaultICMPProtocolSetting (void)
    {
        return &DEFAULT_ICMP_PROTOCOL_SETTING;
    }

}

#endif  // INCL_PROTOCOL_SETTING_H
