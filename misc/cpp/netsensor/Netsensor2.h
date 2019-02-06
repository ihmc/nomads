#ifndef NETSENSOR_NetSensor2__INCLUDED
#define NETSENSOR_NetSensor2__INCLUDED

/*
* NetSensor.h
* Author: rfronteddu@ihmc.us
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
* NetSensor library Class
*/
#include "CommandLineConfigs.h"
#include "ProtoMessageSender.h"
#include "NetsensorStatus.h"
namespace IHMC_NETSENSOR
{
class Netsensor2 : public NOMADSUtil::ManageableThread
{
public:
    Netsensor2 (void);
    ~Netsensor2 (void);
    void run (void);

    /*
    * Returns true if init was successful

    */
    bool init (int argc, char* argv[]);

private:
    bool init (void);
    bool initWithCommandLineCfgs (CommandLineConfigs * pCLC);
    // <------------------------------------------------------------------------------------------>
    ProtoMessageSender _pms;
    NetsensorStatus _ns;
};
}



#endif