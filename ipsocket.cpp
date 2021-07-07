#include "ipsocket.h"

ipsocket::ipsocket()
{
    //memset(m_ipaddr,0,sizeof(m_ipaddr));
    m_ipaddr = NULL;
    m_port = 0;
    m_timeout = 3;
    m_clientsock = -1;
    m_thrstate = THR_Stop;
    m_sockthr = NULL;
    m_isndbuf = 0;
}

ipsocket::~ipsocket()
{
    disconnectFromserver();
    threaddestroy();
    if(m_ipaddr != NULL)
    {
        free(m_ipaddr);
        m_ipaddr = NULL;
    }
}

int ipsocket::setcenter(const char *ip_addr, unsigned short port)
{
    int irtn = 0;

    if(m_ipaddr != NULL)
    {
        free(m_ipaddr);
        m_ipaddr = NULL;
    }

    int strlength = strlen(ip_addr);
    if( strlength > 16)
    {
        printf("ip is invalid(%d)\n",strlength);
        irtn = -1;
        return irtn;
    }

    m_ipaddr = (char *)malloc(sizeof(char) * strlen(ip_addr));
    strcpy(m_ipaddr,ip_addr);
    m_port = port;

    //socket connection check & disconnect
    disconnectFromserver();

    return irtn;
}

int ipsocket::connectToserver(const char *ip_addr, unsigned short port)
{
    int irtn = -1;
    int ierr;

    // create server struct
    struct sockaddr_in svr_addr;
    memset(&svr_addr,0,sizeof(svr_addr));
    ierr = inet_pton(AF_INET,ip_addr,&svr_addr.sin_addr.s_addr);
    if( ierr == 0)
    {
        printf("connect : inet_pton failed(invalid address string)\n");
        return irtn;
    }
    else if (ierr < 0)
    {
        printf("connect : inet_pton failed\n");

        return irtn;
    }
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(port);

    int sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    //set send buffer
    int ircvbuf;
    socklen_t ioptlen = sizeof(ircvbuf);
    ircvbuf = SOCKET_RXTXBUF;
    setsockopt(sock,SOL_SOCKET, SO_RCVBUF, &ircvbuf, ioptlen);
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &ircvbuf,&ioptlen);
    printf("set socket receive buffer : %d\n",ircvbuf);
    m_ircvbuf = ircvbuf;

    int isndbuf = SOCKET_RXTXBUF;
    ioptlen = sizeof(isndbuf);
    setsockopt(sock,SOL_SOCKET, SO_SNDBUF, &isndbuf, ioptlen);
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &isndbuf,&ioptlen);
    printf("set socket send buffer : %d\n",isndbuf);
    m_isndbuf = isndbuf;

    //set send timeout
    timeval sndtimeout;
    sndtimeout.tv_sec = 0;
    sndtimeout.tv_usec = 500000; //500ms
    ioptlen = sizeof(sndtimeout);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sndtimeout,ioptlen);

    ierr = connect(sock,(struct sockaddr *)&svr_addr, sizeof(svr_addr));
    if( ierr != 0 )
    {
        ierr = errno;
        printf("cannot connect to server, err:%d\n",ierr);
        if(sock >= 0)
        {
           close(sock);
           m_clientsock = -1;
           //memcpy(&m_clientaddr,0,sizeof(struct sockaddr_in));

        }
        return irtn;

    }

    printf("connected to server %s:%d\n",ip_addr,port);
    irtn = 0;
    m_clientsock = sock;
    memcpy(&m_clientaddr,&svr_addr,sizeof(struct sockaddr_in));

    return irtn;

}

void ipsocket::asyncconnectToServer(const char *ip_addr, unsigned short port)
{
    setcenter(ip_addr,port);
    threadcreate();
}

int ipsocket::disconnectFromserver()
{
    int irtn = -1;

    if(m_clientsock >= 0)
    {
        close(m_clientsock);
        m_clientsock = -1;
        //memcpy(&m_clientaddr,0,sizeof(struct sockaddr_in));
        irtn = 0;
    }

    return irtn;

}

int ipsocket::threadcreate()
{
    int irtn;

    m_thrstate = THR_Start;
    irtn = pthread_create(&m_sockthr,NULL,ipsocket_loop,this);
    if( irtn < 0 )
    {
        printf("thread create failed\n");
    }
    return irtn;
}

int ipsocket::threaddestroy()
{
    int status;
    if( m_thrstate != THR_Stop )
    {
        m_thrstate = THR_Stop;
        pthread_join(m_sockthr,(void **)&status);
        m_sockthr = NULL;
    }
    else
    {
        printf("already thread stop\n");
    }
}

//async connection
void *ipsocket_loop(void* lpParam)
{
    ipsocket *pclass = (ipsocket *)lpParam;
    pclass->m_thrstate = ipsocket::THR_Running;
    if(pclass->m_clientsock < 0 ) //socket disconnect
    {
       pclass->connectToserver(pclass->m_ipaddr,pclass->m_port);
    }
    pclass->m_thrstate = ipsocket::THR_Stop;
}

int ipsocket::senddata(unsigned char *data, int datalength)
{
    int irtn = -1;
    int bytessent = 0;

    if(m_clientsock < 0 )
    {
        printf("senddata : socket closed\n");
    }
    else
    {
        if( datalength > 0)
        {
            bytessent = datalength;
            irtn = 0;
            while(bytessent > 0)
            {
                //int numbytes = send(m_clientsock, data,bytessent,0);
                //PIPE시그널처리
                int numbytes = send(m_clientsock, data,bytessent,MSG_NOSIGNAL);
                if( numbytes < 0)
                { // socket  handle errors;
                    //int ierr = errno;
                    irtn = numbytes;
                    break;
                }
                else if( numbytes == 0)
                { //socket probably closed
                    irtn = numbytes;
                    break;
                }
                irtn += numbytes;

                data += numbytes;
                bytessent -= numbytes;
            }
        }
    }

    return irtn;
}

int ipsocket::receivedata(unsigned char **data)
{
    int irtn = -1;

    if(m_clientsock < 0)
    {
        printf("receivedata :socket closed\n");
    }
    else
    {
        int numbytes = recv(m_clientsock,m_rdata,SOCKET_RBUFSIZE - 1,0);
        if( numbytes < 0)
        {
            printf("socket receivedata failed\n");
        }
        else if ( numbytes == 0)
        {
            printf("socket connection closed\n");
            disconnectFromserver();
        }
        else
        {
           *data = m_rdata;
        }
        irtn = numbytes;
    }
    return irtn;
}

