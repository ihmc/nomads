/*
 * CommandProcessor.h
 *
 * This file is part of the IHMC Util Library
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
 */

#ifndef INCL_COMMAND_PROCESSOR_H
#define INCL_COMMAND_PROCESSOR_H

#include "ManageableThread.h"
#include "StrClass.h"

namespace NOMADSUtil
{
    class CommHelper2;
    class NetworkServer;
    class Socket;
    class TCPSocket;

    class CommandProcessor : public ManageableThread
    {
        public:
            CommandProcessor (void);
            virtual ~CommandProcessor (void);

            void setPrompt (const char *pszPrompt);

            int enableNetworkAccess (uint16 ui16Port);

            int print (const void *pToken, const char *pszFmt, ...);

            // The following method is invoked by the CommandProcessor upon a non-empty command being entered
            // All whitespace preceding and following the command is trimmed prior to being passed in to this method
            // Command processor will terminate if this method returns a negative value (for example,
            //     the subclass may want to do this in response to "quit" or "exit")
            // Output should be generated using the print() method - with the pToken passed in as an opaque object
            // NOTE: The method is allowed to change pszCmdLine (for example, for parsing purposes)
            //       but the string should not be deallocated
            virtual int processCmd (const void *pToken, char *pszCmdLine) = 0;

            void run (void);

        protected:
            friend class CmdProcConnHandler;
            const char * getPrompt (void);
            static void trim (char *pszBuf);

        protected:
            String _prompt;
            NetworkServer *_pNetworkServer;
    };

    class NetworkServer : public ManageableThread
    {
        public:
            NetworkServer (CommandProcessor *pCP);
            ~NetworkServer (void);

            int init (uint16 ui16Port);
            void close (void);

            void run (void);

            virtual void requestTermination (void);
            virtual void requestTerminationAndWait (void);

        private:
            CommandProcessor *_pCP;
            TCPSocket *_pServerSocket;
    };

    class CmdProcConnHandler : public ManageableThread
    {
        public:
            CmdProcConnHandler (Socket *pSocket, CommandProcessor *pCP);
            ~CmdProcConnHandler (void);

            void run (void);

            void print (const char *pszBuf);

        private:
            Socket *_pSocket;
            CommHelper2 *_pCH;
            CommandProcessor *_pCP;

    };
}

#endif   // #ifndef INCL_COMMAND_PROCESSOR_H
