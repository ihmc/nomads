#ifndef NETSENSOR_InterfaceCaptureMode__INCLUDED
#define NETSENSOR_InterfaceCaptureMode__INCLUDED
/*
* InterfaceCaptureMode.h
* Author: amorelli@ihmc.us
* This file is part of the IHMC NetSensor Library/Component
* Copyright (c) 2010-2017 IHMC.
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
* Each InterfaceMonitor is linked to a specific network interface and it uses
* pcap to retrieve packets from the network and then proceeds in enqueuing
* the packets in the packet queue
*
*/

enum class ICaptureMode {ICM_LIVE, ICM_REPLAY};

#endif