/*
 * DisServiceCommandProcessor.h
 *
 * This file is part of the IHMC DisService Library/Component
 * Copyright (c) 2006-2014 IHMC.
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

#ifndef INCL_DISSERVICE_COMMAND_PROCESSOR_H
#define INCL_DISSERVICE_COMMAND_PROCESSOR_H

#include "CommandProcessor.h"

namespace IHMC_ACI
{
    class DisseminationService;

    class DisServiceCommandProcessor : public NOMADSUtil::CommandProcessor
    {
        public:
            DisServiceCommandProcessor (DisseminationService *pDisService);
            ~DisServiceCommandProcessor (void);

            int processCmd (const void *pToken, char *pszCmdLine);

        protected:
            void displayGeneralHelpMsg (const void *pToken);
            void displayHelpMsgForCmd (const void *pToken, const char *pszCmdLine);
            void handleDBCmd (const void *pToken, const char *pszCmdLine);
            void handlePropCmd (const void *pToken, const char *pszCmdLine);
            void handleHasFragmentCmd (const void *pToken, const char *pszCmdLine);
            void handleScreenOutputCmd (const void *pToken, const char *pszCmdLine);

        protected:
            DisseminationService *_pDisService;
    };
}

#endif   // #ifndef INCL_DISSERVICE_COMMAND_PROCESSOR_H
