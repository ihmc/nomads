/*
* launcher.cpp
*
* This file is part of the IHMC Mockets Library/Component
* Copyright (c) 2002-2017 IHMC.
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

#include <stdio.h>
#include "Mocket.h"
#include "ServerMocket.h"
#include "MessageSender.h"
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../util/cpp/Logger.h"

#define checkAndLogMsg if (pLogger) pLogger->logMsg

using namespace NOMADSUtil;

int client(char *argv[])
{
	printf("Client:\n");

	int rc = -1;
    char *addr = argv[2];
    int port = 49153;
	Mocket *client = new Mocket(0, 0, false, true, "client-cert.pem", "client-key.pem");
	while (rc != 0) {
        printf("Trying to connect to: %s - %d\n", addr, port);
		if ((rc = client->connect(addr, port)) == 0) {
			printf("Connection established\n");
			getchar();
		}
		else {
			printf("Connect failed %d, press a key to terminate\n", rc);
			getchar();
			client->close();
			delete(client);
			return 0;
		}
	}

    client->setLocalAddr("Set client addr");
	MessageSender ms = client->getSender(true, true);
    char buf[1024] = {};
    static const char helloStr[]{"hello, world: 0!"};
    while (true) {
        ms.send(helloStr, sizeof(helloStr));
        printf("Sent string \"%s\"; waiting echo reply...\n", helloStr);

        rc = client->receive(buf, sizeof(buf));
        if (rc <= 0) {
            printf("rc = %d; no data received\n", rc);
        }
        else {
            buf[rc] = '\0';
            printf("received: \"%s\"\n", buf);
            if (strcmp(buf, helloStr)) {
                perror("Sent and received strings do not match!!\n");
                client->close();
                delete(client);
                return -1;
            }
        }
    }

    client->close();
    delete(client);
	printf("Press a key to exit the application\n");
	getchar();

	return 0;
}

int server()
{
	printf("Server:\n");
	int rc = 0;
	ServerMocket *server = new ServerMocket(0, 0, false, true, "server-cert.pem", "server-key.pem");
	Mocket* newMocket;

	rc = server->listen(49153);
	printf("Server listen rc: %d\n", rc);
	//while (true) {
	newMocket = server->accept();
	printf("Connection received\n");

    static char buf[1024] = {};
    newMocket->setLocalAddr("Set server addr");
    MessageSender ms = newMocket->getSender(true, true);
    while (true) {
        rc = newMocket->receive(buf, sizeof(buf));
        if (rc <= 0) {
            printf("rc = %d; no data received\n", rc);
            break;
        }

        buf[rc] = '\0';
        printf("received string: \"%s\"; echoing it back to the sender...\n", buf);
        ms.send(buf, rc);
    }
	//}

    newMocket->close();
    delete(newMocket);
    server->close();
    delete(server);

	printf("Press a key to exit the application\n");
	getchar();
	return 0;
}

int windowsTest (char *argv[])
{
    if (!strcmp(argv[1], "s")) {
        return server();
    }

    if (!strcmp(argv[1], "c")) {
        return client(argv);
    }

	return 0;
}

int linuxTest(char *argv[])
{
	if (!strcmp(argv[1], "s")) {
        return server();
	}

	if (!strcmp(argv[1], "c")) {
        return client(argv);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	pLogger = new NOMADSUtil::Logger();
	pLogger->initLogFile("dtlsMocket.log", false);
	pLogger->enableScreenOutput();
	pLogger->enableFileOutput();
	pLogger->setDebugLevel(NOMADSUtil::Logger::L_NetDetailDebug);

    if (argc < 2) {
        printf("Correct usage is %s <c/s> [IP_ADDR]\n", argv[0]);

        return -1;
    }
    if ((argc == 2) && (strcmp(argv[1], "s"))) {
        printf("Correct usage is %s <c/s> [IP_ADDR]\n", argv[0]);

        return -2;
    }

#ifdef WIN32
	return windowsTest(argv);
#else
	return linuxTest(argv);
#endif

}