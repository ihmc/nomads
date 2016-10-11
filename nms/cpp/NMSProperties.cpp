/*
 * NMSProperties.cpp
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2016 IHMC.
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
 * Author: Giacomo Benincasa    (gbenincasa@ihmc.us)
 * Created on June 24, 2015, 3:30 PM
 */

#include "NMSProperties.h"

using namespace NOMADSUtil;

const uint16 NMSProperties::DEFAULT_NMS_PROXY_PORT = (uint16) 56488;
const uint16 NMSProperties::DEFAULT_CMD_PROCESSOR_PORT = (uint16) 4444;

const String NMSProperties::NMS_PROXY_PORT = "nms.proxy.port";
const String NMSProperties::NMS_PROXY_ADDRESS = "nms.proxy.address";
const String NMSProperties::NMS_CMD_PROCESSOR = "nms.cmdProc.enable";
const String NMSProperties::NMS_CMD_PROCESSOR_PORT = "nms.cmdProc.port";

const String NMSProperties::NMS_TRANSMISSION_MODE = "nms.transmission.mode";
const String NMSProperties::NMS_TRANSMISSION_ASYNC = "nms.asyncTransmission";
const String NMSProperties::NMS_TRANSMISSION_UNICAST_REPLY = "nms.transmission.replyViaUnicast";

const String NMSProperties::NMS_REQUIRED_INTERFACES = "nms.transmission.interfaces.required";
const String NMSProperties::NMS_IGNORED_INTERFACES = "nms.transmission.interfaces.ignored";
const String NMSProperties::NMS_OPTIONAL_INTERFACES = "nms.transmission.interfaces.optional";
const String NMSProperties::NMS_PRIMARY_INTERFACE = "nms.transmission.primaryIface";
const String NMSProperties::NMS_USE_MANAGED_INTERFACES = "nms.transmission.interfaces.managed";
const String NMSProperties::NMS_PERIODIC_MULTICAST_GROUP_REJOIN = "nms.transmission.interfaces.multicast.periodicRejoin";

const String NMSProperties::NMS_PORT = "nms.port";
const String NMSProperties::NMS_OUTGOING_ADDR = "nms.transmission.outgoingAddr";
const String NMSProperties::NMS_TTL = "nms.transmission.ttl";
const String NMSProperties::NMS_MTU = "nms.transmission.mtu";

const String NMSProperties::NMS_MSG_VERSION = "nms.transmission.messageVersion";

const String NMSProperties::NMS_DELIVERY_ASYNC = "nms.asyncDelivery";

