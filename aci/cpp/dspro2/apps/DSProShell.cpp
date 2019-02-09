/*
 * DSProShell.cpp
 *
 * This file is part of the IHMC DSPro Library/Component
 * Copyright (c) 2008-2016 IHMC.
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

#include "DSProShell.h"

#include "StrClass.h"
#include "DSProProxy.h"
#include "DSProCmdProcessor.h"

#include "CallbackHandler.h"
#include "Logger.h"

using namespace NOMADSUtil;
using namespace IHMC_ACI;

namespace IHMC_ACI_DSPRO_SHELL
{
    struct Arguments
    {
        Arguments (void);
        ~Arguments (void);

        bool interactiveMode (void);

        uint16 ui16DesiredApplicationId;
        int iPort;
        String addr;
        String filename;
    };

    Arguments::Arguments (void)
        : ui16DesiredApplicationId (CallbackHandler::DSProShell),
          iPort (56487),
          addr ("127.0.0.1")
    {
    }

    Arguments::~Arguments (void)
    {
    }

    bool Arguments::interactiveMode (void)
    {
        return filename.length() <= 0;
    }

    void parseArguments (int argc, char *argv[], Arguments &arguments)
    {
        for (int i = 0; i < argc; i++) {
            String opt (argv[i]);
            if ((opt ^= "-p") || (opt ^= "--port")) {
                arguments.iPort = atoi (argv[++i]);
            }
            else if ((opt ^= "-a") || (opt ^= "--address")) {
                arguments.addr = argv[++i];
            }
            else if ((opt ^= "-b") || (opt ^= "--batch-mode")) {
                arguments.filename = argv[++i];
            }
        }
    }
}

int main (int argc, char *argv[])
{
    Logger *pLogger = new Logger();
    pLogger->enableScreenOutput();
    pLogger->setDebugLevel ("L_Info");

    String Application_Executable_Name = argv[0];
    const char *pszMethodName = "DSProShell::main";

    // Parse arguments
    IHMC_ACI_DSPRO_SHELL::Arguments arguments;
    parseArguments (argc, argv, arguments);

    // Create and initialize proxy
    DSProProxy proxy (arguments.ui16DesiredApplicationId);
    int rc = proxy.init (arguments.addr, arguments.iPort);
    if (rc == 0) {
        checkAndLogMsg (pszMethodName, Logger::L_Info, "DSProShell connected to DSProProxyserver at %s:%d.\n",
                        arguments.addr.c_str(), arguments.iPort);
    }
    else {
        checkAndLogMsg (pszMethodName, Logger::L_SevereError,
                        "DSProShell has failed to initialize. Return code: %d\n", rc);
        return -2;
    }
    // Start proxy thread
    proxy.start();

    // Create application
    DSProCmdProcessor dsproCmdProc (&proxy);

    // Register the listeners implemented in the application with dspro proxy
    uint16 ui16AsignedId;
    if (proxy.registerDSProListener (arguments.ui16DesiredApplicationId, &dsproCmdProc, ui16AsignedId) < 0) {
        return -3;
    }
    if (proxy.registerSearchListener (arguments.ui16DesiredApplicationId, &dsproCmdProc, ui16AsignedId) < 0) {
        return -4;
    }

    if (arguments.interactiveMode()) {
        dsproCmdProc.run();
    }
    else {
        dsproCmdProc.processCmdFile (arguments.filename);
    }

    return 0;
}

