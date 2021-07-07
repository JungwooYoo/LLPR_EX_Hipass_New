#ifndef IPSOCKET_H
#define IPSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <list>

using namespace std;

void *ipsocket_loop(void* lpParam);

class ipsocket
{
public:
    ipsocket();
    ~ipsocket();

    int setcenter(const char *ip_addr, unsigned short port);
    int connectToserver(const char *ip_addr, unsigned short port);
    void asyncconnectToServer(const char *ip_addr, unsigned short port);
    int disconnectFromserver();
    //loop
    int threadcreate();
    int threaddestroy();

    int senddata(unsigned char *data,int datalength);
    int receivedata(unsigned char **data);
//    int parsing();

    enum ThreadState
    {
        THR_Start,
        THR_Running,
        THR_Stop
    };

public:
    //socket config
    int m_clientsock;
    struct sockaddr_in m_clientaddr;
    char *m_ipaddr;
    unsigned short m_port;
    unsigned int m_timeout;
    int m_isndbuf;
    int m_ircvbuf;

    int m_thrstate;
    pthread_t m_sockthr;
#define SOCKET_RBUFSIZE     1024
#define SOCKET_RXTXBUF      400 * 1024  //400MB
    unsigned char m_rdata[SOCKET_RBUFSIZE];

};

#endif // IPSOCKET_H
