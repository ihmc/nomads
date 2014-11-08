#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "queuemanager.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "NLFLib.h"
#include <unistd.h>
#include <string.h>

QueueManager *qm;

extern "C" void* execThread (void*arg) {
    qm->execQueueManager();
}

int main (int argc, char**argv){
    if (argc != 2) {
        printf ("Usage: %s <ip>\n", argv[0]);
        return (-1);
    }
    int start[1], end[1], rand[1], phase = 2;
    struct in_addr ip[1];
    char com[255];
    inet_aton (argv[1], ip);
    int64 tstart = getTimeInMilliseconds();
    pthread_t id;
    
    system ("/sbin/iptables --flush");
    strcpy (com, "/sbin/iptables -A INPUT -s ");
    strcat (com, argv[1]);
    strcat (com, " -j QUEUE");
    system (com);
            
    rand[0] = 100;
    start[0] = 20;
    end[0] = 20;
    qm = new QueueManager (ip, start, end, rand, 1);
    
    pthread_create(&id, NULL, execThread, NULL);
//    pthread_detach(id);
    
    while (getTimeInMilliseconds() <= (tstart + 80000)) {
        sleep (2);
        if (start[0] == 40) {
            phase = -2;
        }
        start[0] += phase;
        end[0] += phase;      
        qm->configQueueManager (ip, start, end, rand, 1);
    }
    system ("/sbin/iptables --flush");
    return 0;
}
