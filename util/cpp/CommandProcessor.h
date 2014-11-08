/*
 * CommandProcessor.h
 *
 * This file is part of the IHMC Utility Library
 * Copyright (c) IHMC. All Rights Reserved.
 *
 * Usage restricted to not-for-profit use only.
 * Contact IHMC for other types of licenses.
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
            friend class ConnHandler;
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

        private:
            CommandProcessor *_pCP;
            TCPSocket *_pServerSocket;
    };

    class ConnHandler : public ManageableThread
    {
        public:
            ConnHandler (Socket *pSocket, CommandProcessor *pCP);
            ~ConnHandler (void);

            void run (void);

            void print (const char *pszBuf);

        private:
            Socket *_pSocket;
            CommHelper2 *_pCH;
            CommandProcessor *_pCP;

    };
}

#endif   // #ifndef INCL_COMMAND_PROCESSOR_H
