/*
 * NMSCommandProcessor.h
 *
 * This file is part of the IHMC Network Message Service Library
 * Copyright (c) 1993-2014 IHMC.
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
 * Created on March 1, 2015, 6:31 PM
 */

#ifndef INCL_NMS_COMMAND_PROCESSOR_H
#define INCL_NMS_COMMAND_PROCESSOR_H

#include "CommandProcessor.h"

namespace NOMADSUtil
{
    class NetworkMessageService;

    class NMSCommandProcessor : public CommandProcessor
    {
        public:
            explicit NMSCommandProcessor (NetworkMessageService *pNMS);
            virtual ~NMSCommandProcessor (void);

            int processCmd (const void *pToken, char *pszCmdLine);

        private:
            void displayPongMsg (const void *pToken);
            void getEncryptionKey (const void *pToken);

        private:
            NetworkMessageService *_pNMS;
    };
}

#endif    /* INCL_NMS_COMMAND_PROCESSOR_H */

