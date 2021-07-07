#include "centerclient.h"
#include "commonvalues.h"
#include <QApplication>

#include <iconv.h>

CenterClient::CenterClient(int id, QString centerName, int loglevel, QThread *parent) : QThread(parent)
{

//    QString path = QApplication::applicationDirPath();
//    if( !commonvalues::FTPSavePath.isNull() && !commonvalues::FTPSavePath.isEmpty())
//    {
//        path = commonvalues::FTPSavePath;
//    }
//    FTP_SEND_PATH = path + "/FTP_Trans";
    m_centerName = centerName;

    brun = false;
    centerid = id;
    m_prdata = NULL;
    m_prdatalen = 0;
    m_transstate = "START";
    m_kaminterval = 1800; //sec
    m_bconnflag = false;
    setconnectioncount = 5;
    codec = QTextCodec::codecForName("eucKR");
    encoderw = codec->makeEncoder( QTextCodec::IgnoreHeader);

    fileNameSelect = CenterInfo::H_Char;
    protocol_type = CenterInfo::Normal;

    m_workstartRetry=0;
    m_workstartflag=0;

    m_pftp = NULL;
    m_pftpfile = NULL;
    m_iFiledownloadCnt=0;
    m_bDownload =false;
    //m_bOBU_Update = false;
    m_OBUFileName = "";

    m_pftp = new QFtp(this);
    connect(m_pftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));

    log = new Syslogger(this,"centerthread",true,loglevel);
    m_loglevel = loglevel;
    this->start();

}

CenterClient::~CenterClient()
{    
    log->deleteLater();
}

void CenterClient::SetLogLevel(int loglevel)
{
     log->m_loglevel = loglevel;
     m_loglevel = loglevel;
}

void CenterClient::run()
{
    brun = true;
    int loopcount=0;
    int kamloopcount=0;
    int oldclientsock=0;

    QDateTime lastalivetime = QDateTime::currentDateTime().addDays(-1);
    QDateTime currentTime;

    while(brun)
    {
        if(protocol_type == CenterInfo::Normal)
        {
            //async connection
            if(m_bconnflag)
            {
                if(m_clientsock == -1)//disconnection
                {
                   int irtn =connectToserver(m_ip.toUtf8().constData(),m_port);
                   if( irtn == 0)
                   {
                       QString logstr = QString("Center Connection(%1)").arg(centerid);
                       qDebug() << logstr;      log->write(logstr,LOG_NOTICE);
                   }
                   else
                   {
                       QString logstr = QString("Center Connection Fail(%1)").arg(centerid);
                       qDebug() << logstr;      log->write(logstr,LOG_INFO);
                   }
                }
                m_bconnflag = false;
            }

            //disconnection check
            if(m_clientsock >= 0 && oldclientsock < 0 ) //action connect
            {

                emit Connected();
                connectioncount = setconnectioncount;
                //immediately StatusREP after connection
                lastalivetime = QDateTime::currentDateTime().addDays(-1);
            }
            else if( m_clientsock < 0 && oldclientsock >= 0)
            {
                emit Disconnected(); //action disconnected

            }
            oldclientsock = m_clientsock;
            //receive data check
            receivedatacheck();
            //repeat send data
            if(m_clientsock >= 0)
            {
                if(connectioncount < 0 ) //No receive data
                {
                    Disconnect();
                }

                currentTime = QDateTime::currentDateTime();
                qint64 diffsec = lastalivetime.secsTo(currentTime);
                if(diffsec > m_kaminterval)
                {
                    StatusREP(commonvalues::LaneID);
                    lastalivetime = QDateTime::currentDateTime();
                }
            }
        }
        else if( protocol_type == CenterInfo::Remote )
        {
            //async connection
           if(m_bconnflag)
           {
                if(m_clientsock == -1)//disconnection
                {
                   int irtn =connectToserver(m_ip.toUtf8().constData(),m_port);
                   if( irtn == 0)
                   {
                       QString logstr = QString("Center Connection(%1)").arg(centerid);
                       qDebug() << logstr;      log->write(logstr,LOG_NOTICE);
                   }
                   else
                   {
                       QString logstr = QString("Center Connection Fail(%1)").arg(centerid);
                       qDebug() << logstr;      log->write(logstr,LOG_INFO);
                   }
                }
                m_bconnflag = false;
            }

            //disconnection check
            if(m_clientsock >= 0 && oldclientsock < 0 ) //action connect
            {
                emit Connected();

                WorkStark_Send();

                FwUpdateResult();

                //immediately StatusREP after connection
                lastalivetime = QDateTime::currentDateTime().addDays(-1);
            }
            else if( m_clientsock < 0 && oldclientsock >= 0)
            {
                emit Disconnected(); //action disconnected

            }
            oldclientsock = m_clientsock;
            //receive data check
            receivedatacheck();
            //repeat send data
            if(m_clientsock >= 0)
            {
//                if(connectioncount < 0 ) //No receive data
//                {
//                    Disconnect();
//                }
                if(m_workstartflag > 0)
                {
                    WorkStark_Send();
                }

                //차량데이터전송
                if(remoteCarDatalist.count() > 0 )
                {
                    clsHipassCarData *cardata = remoteCarDatalist.takeFirst();
                    CarNoDataHipass_Send(cardata);
                    if(cardata != NULL )
                    {
                        delete cardata;
                        cardata = NULL;
                    }
                }

                currentTime = QDateTime::currentDateTime();
                qint64 diffsec = lastalivetime.secsTo(currentTime);
                if(diffsec > m_kaminterval)
                {
                    LaneStatus_Send();  // 차로상태전송,50초 주기
                    lastalivetime = QDateTime::currentDateTime();
                }
            }
        }
        else //FTP_Only
        {
            if(m_clientsock >= 0)
            {
                Disconnect();
            }

        }

        msleep(30);
    }
}

void CenterClient::stop()
{
    brun = false;
}

void CenterClient::receivedatacheck()
{
    int MaxNumSockets = -1;
    //timeout
    timeval tvtimeout;
    tvtimeout.tv_sec = 0;
    tvtimeout.tv_usec = 0;

    fd_set readset;
    FD_ZERO(&readset);

    //socket check
    if(m_clientsock >= 0)
    {
        FD_SET(m_clientsock,&readset);
        if(m_clientsock >= MaxNumSockets)
        {
            MaxNumSockets = m_clientsock + 1;
        }
    }

    if(MaxNumSockets < 0) return;
    //receive data check
    int selectResult = select(MaxNumSockets, &readset,NULL,NULL,&tvtimeout);
    if( selectResult < 0)
    {
        int err = errno;
        if(err != 0)
        {

        }
    }
    //action
    else if(FD_ISSET(m_clientsock,&readset))
    {
        connectioncount = setconnectioncount;
        if(protocol_type == CenterInfo::Remote)
            RemoteParsing();
        else
            Parsing();
    }
}

bool CenterClient::Connect(QString ip, QString port, QString centerName, int mkaminterval)
{
    m_ip = ip;
    m_port = (ushort)port.toUInt();
    m_kaminterval = mkaminterval;
    m_centerName = centerName;

    QString logstr;
    int irtn = connectToserver(m_ip.toUtf8().constData(),m_port);
    if( irtn != 0)
    {
        logstr = QString("Center Connection Fail(%1)").arg(centerid);
        qDebug() << logstr;   log->write(logstr,LOG_INFO);
        return false;
    }

    logstr = QString("Center Connection(%1)").arg(centerid);
    qDebug() << logstr;   log->write(logstr,LOG_NOTICE);
    return true;
}

void CenterClient::ConnectAsync(QString ip, QString port, QString centerName, int mkaminterval)
{
    m_ip = ip;
    m_port = (ushort)port.toUInt();
    m_centerName = centerName;
    m_kaminterval = mkaminterval;
    m_bconnflag = true;
}

bool CenterClient::Disconnect()
{
    QString logstr;
    bool brtn = false;
    disconnectFromserver();
    if(m_clientsock < 0)
    {
        brtn = true;
        logstr = QString("Center Disconnection(%1)").arg(centerid);
        qDebug() << logstr;   log->write(logstr,LOG_NOTICE);

    }
    else
    {
        logstr = QString("Center Disconnection Fail(%1)").arg(centerid);
        qDebug() << logstr;   log->write(logstr,LOG_NOTICE);
    }

    return brtn;
}

int CenterClient::Parsing()
{
    int irtn = 0;

     unsigned char *data;
     int datalen = receivedata(&data);

     if( datalen > 0 && commonvalues::loglevel >= LOG_DEBUG)
     {
         int receivelen = datalen;
         QByteArray printdata((const char*)data,receivelen);
         QString logstr = QString("ReceivedData(%1)-len(%2): %3")
                 .arg(centerid).arg(datalen).arg(QString(printdata.toHex()));
         qDebug() << logstr;
         log->write(logstr,LOG_DEBUG);
     }

     if( datalen == 0 )// disconnection
     {
         Disconnect();
         return irtn;
     }
     else if( datalen < 0 ) // error
     {
         irtn = datalen;
         return irtn;
     }     

     irtn = ParsePacket(data,datalen);
     return irtn;

}

int CenterClient::ParsePacket(unsigned char *data, int datalen)
{
    int irtn = -1;
    QByteArray Command((const char*)data,datalen);
    try
    {
          if( Command.length() < 1 ) return irtn;

          char cmd = Command[0];
          ///명령어 처리구문 있어야함.
          // 초기접속
          if (cmd == '#')
          {
              InitConnection(commonvalues::LaneID);
          }
          //상태요구
          else if (cmd == 'S')
          {
              //StatusREP(iLPR_EX_HIPASS.Config.iLPR.LaneID);
              Status_Send();
          }
          //버전요구
          else if (cmd == 'V')
          {
              LaneVersion(0);
          }
          //차로리셋
          else if (cmd == 'R')
          {
              //iLPR restart
              emit CommandEvent(centerid,"R",NULL);
          }
          //차종요구
          else if (cmd == 'H')
          {
              VehicleTypeReqREP(commonvalues::Check_CarType);
          }
          //차종변경
          else if (cmd == 'A')
          {
              QString strcommand = QString(Command);
              strcommand.remove(0,1);  //remove 'A'
              //QStringList lanelist =  strcommand.split(',');

              QString logstr = QString("차종요구(%1)  : %2")
                      .arg(centerid).arg(strcommand);
              qDebug() << logstr;
              log->write(logstr,LOG_DEBUG);
          }
          //차로날짜변경
          else if (cmd == 'T')
          {
              QString strcommand = QString(Command);
              strcommand.remove(0,1);  //remove 'T'

             emit CommandEvent(centerid,"T",strcommand);  //strcommand : yyyyMMddHHmmss

          }
          //현재차로날짜
          else if (cmd == 'G')
          {
              CurrentDateREP();
          }
          irtn = 0;
    }
    catch( ... )
    {
        QString logstr = QString("CenterClient(%1) Parsing Error : %2")
                .arg(centerid).arg(QString(Command.toHex()));
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
    }
    return irtn;
}

bool CenterClient::SendData(QString strdata)
{
    QByteArray bcmd = strdata.toUtf8();
    if( senddata((unsigned char *)bcmd.data(),bcmd.length()) > 0)
    {
        return true;
    }
    return false;
}

bool CenterClient::InitConnection(uint laneID)
{
    QString Command = QString("I%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::StatusREP(uint laneID)
{
    QString Command = QString("S%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::LaneVersion(uint version)
{
    QString Command = QString("V%1").arg(version,4,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::VehicleTypeReqREP(QStringList CarType)
{
    if( CarType.size() < 0) return false;

    QString Command = "H";
    for( int i = 0; i < CarType.size(); i++)
    {
        if(!CarType.isEmpty())
        {
            if( Command.length() > 1) Command += ",";
            Command += CarType[i];
        }
    }
    return SendData(Command);
}

bool CenterClient::VehicleTypeChangeREP(QStringList CarType)
{
    if( CarType.size() < 0) return false;

    QString Command = "A";
    for( int i = 0; i < CarType.size(); i++)
    {
        if(!CarType.isEmpty())
        {
            if( Command.length() > 1) Command += ",";
            Command += CarType[i];
        }
    }
    return SendData(Command);
}

bool CenterClient::CurrentDateREP()
{
    QString Command = QString("G%1").arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
    return SendData(Command);
}

bool CenterClient::CCUCommStatus(bool bstatus, uint laneID)
{
    QString Command;
    if( bstatus ) //정상
        Command =  QString("O%1").arg(laneID,3,10,QChar('0'));
    else
        Command =  QString("B%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::CameraPortStatus(bool bstatus, uint laneID)
{
    QString Command;
    if( bstatus ) //정상
        Command =  QString("F%1").arg(laneID,3,10,QChar('0'));
    else
        Command =  QString("X%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::LightStatus(bool bstatus, uint laneID)
{
    QString Command;
    if( bstatus ) //정상
        Command =  QString("W%1").arg(laneID,3,10,QChar('0'));
    else
        Command =  QString("E%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

bool CenterClient::BoothStatus(bool bstatus, uint laneID)
{
    QString Command;
    if( bstatus ) //정상
        Command =  QString("K%1").arg(laneID,3,10,QChar('0'));
    else
        Command =  QString("C%1").arg(laneID,3,10,QChar('0'));
    return SendData(Command);
}

void CenterClient::Status_Send()
{
    uint laneID  = commonvalues::LaneID;

    CCUCommStatus(commonvalues::localdevstatus[0].ccu > 0 ? false : true,laneID);
    LightStatus(commonvalues::localdevstatus[0].light > 0 ? false : true,laneID);
    CameraPortStatus(commonvalues::localdevstatus[0].camera > 0 ? false : true, laneID);
}


int CenterClient::RemoteParsing()
{
    int irtn = 0;
    QString logstr;

    try
    {
         unsigned char *data;
         int datalen = receivedata(&data);

         if( datalen > 0 && commonvalues::loglevel >= LOG_DEBUG)
         {
             int receivelen = datalen;
             QByteArray printdata((const char*)data,receivelen);
             QString logstr = QString("ReceivedData(%1)-len(%2): %3")
                     .arg(centerid).arg(datalen).arg(QString(printdata));
             qDebug() << logstr;
             log->write(logstr,LOG_DEBUG);
         }

         if( datalen == 0 )// disconnection
         {
             //Disconnect();
             return irtn;
         }
         else if( datalen < 0 ) // error
         {
             irtn = datalen;
             return irtn;
         }

         int pdatalen = m_prdatalen + datalen;
         unsigned char *pdata = (unsigned char *)malloc(sizeof(unsigned char)* pdatalen);
         if( pdata == NULL)
         {
             QString logstr = QString("before Parsing - malloc fail(%1)").arg(centerid);
             log->write(logstr,LOG_ERR);
             return -1;
         }

         if( m_prdatalen > 0 ) memcpy(pdata, m_prdata, sizeof(unsigned char) * m_prdatalen);
         memcpy(pdata + m_prdatalen,data,sizeof(unsigned char) * datalen);

         if( m_prdata != NULL)
         //if(m_prdatalen > 0)
         {
             free(m_prdata);
             m_prdatalen = 0;
             m_prdata=NULL;
         }

         int index = 0;
         int length = pdatalen - index;


         while( length > PROTOCOL_FILESIZE  )
         {

             unsigned int filesize = 0;
             if( length > 5 )
             {
                 QByteArray bfsize((const char*)pdata,PROTOCOL_FILESIZE);
                 filesize = bfsize.toUInt();
             }
             int rdatalen = index + filesize + PROTOCOL_FILESIZE;
             if( rdatalen > pdatalen ) break;

             QByteArray message( (char *)(pdata+ index + PROTOCOL_FILESIZE),filesize);

             bool brtn = Analysis(message);
             if(!brtn)
                 Disconnect();

             index += filesize + PROTOCOL_FILESIZE;
             length = pdatalen - index;
         }

         if( length > 0 )
         {
             m_prdata = (unsigned char *)malloc( sizeof(unsigned char) * length );
             if( m_prdata != NULL )
             {
                 memcpy(m_prdata, pdata + index, sizeof(unsigned char) * length);
                 m_prdatalen = length;
             }
         }

         if( pdata != NULL)
         {
             free(pdata);
             pdatalen = 0;
             pdata = NULL;
         }
    }
    catch ( ... )
    {
        logstr = QString("---------CenterClient(%1) RemoteParsing Error----------")
                .arg(centerid);
        qDebug() << logstr;
        if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logcenter,logstr); //plogdlg1->logappend(logstr);
        log->write(logstr,LOG_ERR);
    }
    return irtn;
}

bool CenterClient::Analysis(QByteArray orgdata)
{
    bool brtn = false;
    QString logstr;

    try
    {
        if(orgdata.isNull())
            return brtn;

        //parse json
        iconv_t  it;
        it = iconv_open("UTF-8","EUC-KR");

        mempcpy(reuckr_buf,orgdata.data(),(size_t)orgdata.length());
        memset(rutf8_buf,'\0',REMOTEDATA_BUF);
        size_t out_size = sizeof(rutf8_buf);
        size_t in_size = strlen(orgdata.data());//(size_t)ByteData.length();//strlen(ByteData.data());
        char* input_buf_ptr = reuckr_buf;//ByteData.data();
        char* output_buf_ptr = rutf8_buf;

        int ret = iconv(it,&input_buf_ptr,&in_size,&output_buf_ptr,&out_size);

        if( ret < 0)
        {
            QString logstrerr = QString("----Analysis Data convert(euc-kr->utf8) Error(%1)----").arg(centerid);
            log->write(logstr,LOG_ERR); qDebug() << logstrerr;
            iconv_close(it);
            return brtn;
        }

        out_size = strlen(rutf8_buf);
        QByteArray data = QByteArray(rutf8_buf,out_size);
        iconv_close(it);

        QJsonDocument jMessage = QJsonDocument::fromJson(QString::fromUtf8(data).toUtf8());
        QJsonObject jRootObject = jMessage.object();
        QVariantMap jRootMap = jRootObject.toVariantMap();

        QString sMSG_TYPE = jRootMap.value("MSG_TYPE").toString();
        QString sMAKER_NAME = jRootMap.value("MAKER_NAME").toString();

        if( sMSG_TYPE.isNull())
        {
            logstr = QString("MSG_TYPE is NULL(id:%1)")
                    .arg(centerid);
            plogdlg0->logappend(LogDlg::logcenter,logstr); qDebug() << logstr;
            log->write(logstr,LOG_ERR);
             return brtn;
        }
        else if( sMAKER_NAME.compare("JWIN") != 0 )
        {
            logstr = QString("MAKER_NAME is mismatching : %1").arg(sMAKER_NAME);
            qDebug() << logstr;
            log->write(logstr,LOG_ERR);
            return brtn;
        }

        if( sMSG_TYPE.compare("SYSTEM_REBOOT_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sSystem_Reboot = jData["SYSTEM_REBOOT"].toString();

            QString strCommand = QString("%1,%2,%3").arg(sControl_No).arg(sReq_Datetime).arg(sSystem_Reboot);

            emit CommandEvent(centerid,sMSG_TYPE,strCommand);

            QString returndata = QString("%1,%2").arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
            CommandRESP("CONTROL","SYSTEM_REBOOT_RESP",returndata);

        }
        else if( sMSG_TYPE.compare("CAMERA_REBOOT_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sCamera_Reboot = jData["CAMERA_REBOOT"].toString();

            QString strCommand = QString("%1,%2,%3").arg(sControl_No).arg(sReq_Datetime).arg(sCamera_Reboot);

            emit CommandEvent(centerid,sMSG_TYPE,strCommand);

            QString returndata = QString("%1,%2").arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
            CommandRESP("CONTROL","CAMERA_REBOOT_RESP",returndata);

        }
        else if( sMSG_TYPE.compare("ZOOM_CONTROL_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sCAM_NUMBER = jData["CAM_NUMBER"].toString();
            QString sZoom_Control = jData["ZOOM_CONTROL"].toString();

            QString strCommand = QString("%1,%2,%3,%4").arg(sControl_No).arg(sReq_Datetime).arg(sCAM_NUMBER).arg(sZoom_Control);

            emit CommandEvent(centerid,sMSG_TYPE,strCommand);

            QString returndata = QString("%1,%2,%3")
                    .arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                    .arg(LENS_CONTROL_TIME);
            CommandRESP("CONTROL","ZOOM_CONTROL_RESP",returndata);
        }
        else if( sMSG_TYPE.compare("FOCUS_CONTROL_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sCAM_NUMBER = jData["CAM_NUMBER"].toString();
            QString sFocus_Control = jData["FOCUS_CONTROL"].toString();

            QString strCommand = QString("%1,%2,%3,%4").arg(sControl_No).arg(sReq_Datetime).arg(sCAM_NUMBER).arg(sFocus_Control);

            emit CommandEvent(centerid,sMSG_TYPE,strCommand);

            QString returndata = QString("%1,%2,%3")
                    .arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                    .arg(LENS_CONTROL_TIME);
            CommandRESP("CONTROL","FOCUS_CONTROL_RESP",returndata);
        }
        else if( sMSG_TYPE.compare("IRIS_CONTROL_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sCAM_NUMBER = jData["CAM_NUMBER"].toString();
            QString sIris_Control = jData["IRIS_CONTROL"].toString();

            QString strCommand = QString("%1,%2,%3,%4").arg(sControl_No).arg(sReq_Datetime).arg(sCAM_NUMBER).arg(sIris_Control);

            emit CommandEvent(centerid,sMSG_TYPE,strCommand);

            QString returndata = QString("%1,%2,%3")
                    .arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                    .arg(LENS_CONTROL_TIME);
            CommandRESP("CONTROL","IRIS_CONTROL_RESP",returndata);
        }
        else if( sMSG_TYPE.compare("FW_UPDATE_REQ") == 0)
        {
            //firmware name => 제조사_보드명_버전규칙
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sFW_FTP_IP = jData["FW_FTP_IP"].toString();
            QString sFW_FTP_PORT = jData["FW_FTP_PORT"].toString();
            QString sFW_FTP_ID = jData["FW_FTP_ID"].toString();
            QString sFW_FTP_PASS = jData["FW_FTP_PASS"].toString();
            QString sREQ_DATETIME = jData["REQ_DATETIME"].toString();
            QString sFW_DIV = jData["FW_DIV"].toString();
            QString sFW_VERSION = jData["FW_VERSION"].toString();
            QString sFW_IMAGE_PATH = jData["FW_IMAGE_PATH"].toString();
            QString sFW_IMAGE_FILE_NAME = jData["FW_IMAGE_FILE_NAME"].toString();

            QDateTime sREQTime = QDateTime::currentDateTime();

            if( FTPDownload(sFW_FTP_IP,sFW_FTP_PORT,sFW_FTP_ID,sFW_FTP_PASS,sFW_IMAGE_PATH,sFW_IMAGE_FILE_NAME))
            {
                QString returndata = QString("%1,%2")
                        .arg(sControl_No).arg(sREQTime.toString("yyyyMMddHHmmss"));
                CommandRESP("CONTROL","FW_UPDATE_RESP",returndata);

                QString savedata = QString("%1,%2").arg(sControl_No).arg(sFW_IMAGE_FILE_NAME);
                emit CommandEvent(centerid,sMSG_TYPE,savedata);
            }
            else
            {
                //펌웨어를 다운로드만 받고 적용은 안함.
                QString returndata2 = QString("%1,%2,%3,%4,%5")
                        .arg(sControl_No).arg(sFW_DIV).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                        .arg("미적용").arg(sFW_IMAGE_FILE_NAME);
                CommandRESP("CONTROL","FW_UPDATE_RESULT",returndata2);
            }

        }
        else if( sMSG_TYPE.compare("OBU_MACH_UPDATE_REQ") == 0)
        {
            QVariantMap jData = jRootMap[sMSG_TYPE].toMap();

            QString sControl_No = jData["CONTROL_NO"].toString();
            QString sOBU_FTP_IP = jData["OBU_FTP_IP"].toString();
            QString sOBU_FTP_PORT = jData["OBU_FTP_PORT"].toString();
            QString sOBU_FTP_ID = jData["OBU_FTP_ID"].toString();
            QString sOBU_FTP_PASS = jData["OBU_FTP_PASS"].toString();
            QString sREQ_DATETIME = jData["REQ_DATETIME"].toString();
            QString sMACH_FILE_PATH = jData["MACH_FILE_PATH"].toString();
            QString sMACH_FILE_NAME = jData["MACH_FILE_NAME"].toString();

            QDateTime sREQTime = QDateTime::currentDateTime();

            if( OBUFTPDownload(sOBU_FTP_IP,sOBU_FTP_PORT,sOBU_FTP_ID,sOBU_FTP_PASS,sMACH_FILE_PATH,sMACH_FILE_NAME))
            {                
                QString returndata = QString("%1,%2")
                        .arg(sControl_No).arg(sREQTime.toString("yyyyMMddHHmmss"));
                CommandRESP("CONTROL","OBU_MACH_UPDATE_RESP",returndata);

                QString strCommand = QString("%1,%2").arg(sControl_No).arg(sMACH_FILE_NAME);

                if(commonvalues::obuMatch > 0)
                        emit CommandEvent(centerid,sMSG_TYPE,strCommand);
                else
                { //OBU DB insert 안함.
                    m_OBUFileName = QString(sMACH_FILE_NAME.data(),sMACH_FILE_NAME.size());
                    bool bupdate = false;//OBUUpdate(sMACH_FILE_NAME);
                    QString returndata2 = QString("%1,%2,%3,%4")
                            .arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                            .arg(bupdate ? "적용" : "미적용").arg(sMACH_FILE_NAME);
                    CommandRESP("CONTROL","OBU_MACH_UPDATE_RESULT",returndata2);
                }

            }
            else //download failed
            {
                bool bupdate = false;//OBUUpdate(sMACH_FILE_NAME);
                QString returndata2 = QString("%1,%2,%3,%4")
                        .arg(sControl_No).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                        .arg(bupdate ? "적용" : "미적용").arg(sMACH_FILE_NAME);
                CommandRESP("CONTROL","OBU_MACH_UPDATE_RESULT",returndata2);
            }



        }
        else if( sMSG_TYPE.compare("RESULT_CONFIRM_RESP") == 0)
        {
            QVariantMap jData = jRootMap["RESULT_CONFIRM_RESP"].toMap();

            QString sReq_Datetime = jData["REQ_DATETIME"].toString();
            QString sRESP_MSG_TYPE = jData["RESP_MSG_TYPE"].toString();

            logstr = QString("RRESULT_CONFIRM_RESP : %1/%2").arg(sReq_Datetime).arg(sRESP_MSG_TYPE);
            log->write(logstr,LOG_INFO); qDebug() << logstr;
        }
        else
        {
            logstr = QString("Received No command : %1").arg(sMSG_TYPE);
            if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logcenter,logstr); qDebug() << logstr;
            log->write(logstr,LOG_NOTICE);
            return brtn;
        }

        brtn = true;
    }
    catch( ... )
    {
        logstr = QString("--------CenterClient(%1) Analysis Error : %2----------")
                .arg(centerid).arg(QString(orgdata));
        qDebug() << logstr;
//        logappend(logstr);
        log->write(logstr,LOG_ERR);

        brtn = false;
    }

    return brtn;
}

QJsonObject CenterClient::MakeCommon(QString datatype, QString msgtype)
{
    QJsonObject jRootObject;
    jRootObject.insert("DATA_TYPE",datatype); //COLLECT/CONTROL
    jRootObject.insert("SYS_TYPE",commonvalues::Remotedata->SYS_TYPE);
    jRootObject.insert("SEND_TIME",QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz"));
    jRootObject.insert("IC_CODE",commonvalues::Remotedata->IC_CODE);
    jRootObject.insert("LANE_NO",commonvalues::Remotedata->LANE_NO);
    jRootObject.insert("BD_NAME",commonvalues::Remotedata->BD_NAME);
    jRootObject.insert("MSG_TYPE",msgtype);
    jRootObject.insert("MAKER_NAME",commonvalues::Remotedata->MAKER_NAME);
    jRootObject.insert("INTERFACE_VERSION",commonvalues::Remotedata->INTERFACE_VERSION);
    jRootObject.insert("FW_H_VERSION",commonvalues::Remotedata->FW_H_VERSION);

    return jRootObject;
}

bool CenterClient::jSendData(QJsonObject jdata)
{
    wmutex.lock();

    QString logstr;    
    bool brtn = false;
    int isenddatalen=0;
    QByteArray sdata;

    try
    {
        //data merge
        QJsonDocument jMessage(jdata);
        QByteArray ByteData = jMessage.toJson();//QJsonDocument::Compact);  //  QJsonDocument::Commpact , Indented
        iconv_t  it;
        it = iconv_open("EUC-KR","UTF-8");

        mempcpy(tutf8_buf,ByteData.data(),(size_t)ByteData.length());
        memset(teuckr_buf,'\0',REMOTEDATA_BUF);
        size_t out_size = sizeof(teuckr_buf);
        size_t in_size = strlen(ByteData.data());//(size_t)ByteData.length();//strlen(ByteData.data());
        char* input_buf_ptr = tutf8_buf;//ByteData.data();
        char* output_buf_ptr = teuckr_buf;

        int ret = iconv(it,&input_buf_ptr,&in_size,&output_buf_ptr,&out_size);

        if( ret < 0)
        {
            QString logstrerr = QString("----jSendData Data convert(utf8->euc-kr) Error(%1)----").arg(centerid);
            log->write(logstr,LOG_ERR); qDebug() << logstrerr;
            iconv_close(it);
            return brtn;
        }

        out_size = strlen(teuckr_buf);

        int ibytedatalen = out_size;// ByteData.length();
        QString sLength = QString("%1").arg(ibytedatalen,5,10,QChar('0'));

        sdata = sLength.toUtf8();
        //sdata.append(ByteData);
        sdata.append(QByteArray(teuckr_buf,out_size));

        if(m_loglevel >= LOG_DEBUG)
        {
            log->write(QString(sdata),LOG_DEBUG);
        }

        isenddatalen = sdata.length();

        iconv_close(it);
    }
    catch( ... )
    {
        QString logstrerr = QString("----jSendData Data merge Error(%1)----").arg(centerid);
        log->write(logstr,LOG_ERR); qDebug() << logstrerr;
        wmutex.unlock();
        return brtn;
    }


    try
    {
        if( isenddatalen > 0)
        {
            int numbytes = senddata((unsigned char *)sdata.constData(), isenddatalen);
            if( numbytes < 0)
            {
                //socket handle error
                if(m_clientsock > 0)
                {
                    int ierr = errno;
                    logstr = QString("SendData(%1):Socket Send Error(%2)").arg(centerid).arg(ierr);
                }
                Disconnect();
            }
            else if( numbytes == 0)
            {
                //socket closed
                Disconnect();
                logstr = QString("SendData(%1):socket closed").arg(centerid);
            }
            else if( numbytes != isenddatalen )
            {
                // socket fail
                logstr = QString("SendData(%1):socket send failed(%2/%3)").arg(centerid).arg(numbytes).arg(isenddatalen);
            }
            else
            {
                //send success;
                brtn = true;
            }
            //m_connectioncount--;
        }
        else
        {
            logstr = QString("SendData - Zero(%1)").arg(centerid);
        }
    }
    catch( ... )
    {
        QString logstrerr = QString("----jSendData Error(%1)----").arg(centerid);
        log->write(logstrerr,LOG_ERR);  qDebug() << logstrerr;
    }
    wmutex.unlock();

    if(!logstr.isNull() && !logstr.isEmpty()) log->write(logstr,LOG_INFO);

    return brtn;
}

bool CenterClient::WorkStark_Send()
{
    bool brtn = false;
    m_workstartflag = 0;

    if(commonvalues::Remotedata->WORK_START_DATETIME.isEmpty())
    {
        QString logstr = QString("WORK_START_DATETIME is Empty(%1)").arg(centerid);
        qDebug() << logstr;      log->write(logstr,LOG_NOTICE);

        m_workstartRetry++;

        return brtn;
    }

    QString msgtype = "WORK_START";
    QString datatype = "COLLECT";
    QJsonObject jRootObject = MakeCommon(datatype,msgtype);
    QJsonObject jWorkStart;
    jWorkStart.insert("WORK_START_DATETIME",commonvalues::Remotedata->WORK_START_DATETIME);
    jWorkStart.insert("LANE_TYPE",commonvalues::Remotedata->LANE_TYPE);
    jWorkStart.insert("LANE_SYS_TYPE",commonvalues::Remotedata->LANE_SYS_TYPE);

    jRootObject.insert(msgtype,jWorkStart);

    brtn = jSendData(jRootObject);
    if( !brtn )
    {
        QString logstr = QString("WorkStark_Send Error(%1)").arg(centerid);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("WorkStark_Send(%1) : Work-%2,TYPE-%3,SYS_TYPE-%4")
                .arg(centerid).arg(commonvalues::Remotedata->WORK_START_DATETIME)
                .arg(commonvalues::Remotedata->LANE_TYPE).arg(commonvalues::Remotedata->LANE_SYS_TYPE);
        log->write(logstr,LOG_INFO); qDebug() << logstr;
    }

    return brtn;
}

bool CenterClient::LaneStatus_Send()
{
    bool brtn = false;

    QString msgtype = "LANE_STATUS";
    QString datatype = "COLLECT";
    QJsonObject jRootObject = MakeCommon(datatype,msgtype);
    QJsonObject jLaneStatus;
    jLaneStatus.insert("LANE_KIND",commonvalues::Remotedata->LANE_KIND);
    jLaneStatus.insert("MANUFACTURER",commonvalues::Remotedata->MANUFACTURER);
    jLaneStatus.insert("MANU_YEAR",commonvalues::Remotedata->MANU_YEAR);
    jLaneStatus.insert("FW_VERSION",commonvalues::Remotedata->FW_VERSION);
    jLaneStatus.insert("FW_STATE",commonvalues::Remotedata->FW_STATE);
    jLaneStatus.insert("SYS_FORM_TYPE",commonvalues::Remotedata->SYS_FORM_TYPE);
    jLaneStatus.insert("CAMERA_CNT",QString("%1").arg(commonvalues::Remotedata->cameraST.count(),2,10,QChar('0')));
    jLaneStatus.insert("CONTROLLERCONNECTION_STATE",commonvalues::Remotedata->CONTROLLERCONNECTION_STATE);
    jLaneStatus.insert("FTPCONNECTION_STATE",commonvalues::Remotedata->FTPCONNECTION_STATE);

    QJsonArray jCam_Info;
    int imaxcam = commonvalues::Remotedata->cameraST.count();
    for(int index=0; index < imaxcam; index++)
    {
        clsCameraSTATUS camStatus = commonvalues::Remotedata->cameraST.value(index);
        QJsonObject CamObject;

        CamObject.insert("CAM_NO",camStatus.CAM_NO);
        CamObject.insert("LANE_INDEX",camStatus.LANE_INDEX);
        CamObject.insert("CAM_POSTION",camStatus.CAM_POSTION);
        CamObject.insert("IMAGE_RESOLUTION",camStatus.IMAGE_RESOLUTION);
        CamObject.insert("ZOOMCONTROL_MINRANGE",camStatus.ZOOMCONTROL_MINRANGE);
        CamObject.insert("ZOOMCONTROL_MAXRANGE",camStatus.ZOOMCONTROL_MAXRANGE);
        CamObject.insert("FOCUSCONTROL_MINRANGE",camStatus.FOCUSCONTROL_MINRANGE);
        CamObject.insert("FOCUSCONTROL_MAXRANGE",camStatus.FOCUSCONTROL_MAXRANGE);
        CamObject.insert("IRISCONTROL_MINRANGE",camStatus.IRISCONTROL_MINRANGE);
        CamObject.insert("IRISCONTROL_MAXRANGE",camStatus.IRISCONTROL_MAXRANGE);
        CamObject.insert("CAMERA_STATE",camStatus.CAMERA_STATE);
        CamObject.insert("LENSCONTROLBOARD_STATE",camStatus.LENSCONTROLBOARD_STATE);
        CamObject.insert("DETECTOR_STATE",camStatus.DETECTOR_STATE);
        CamObject.insert("STROBE_STATE",camStatus.STROBE_STATE);

        jCam_Info.push_back(QJsonValue(CamObject));
    }

    jLaneStatus.insert(QString("CAM_INFO"),QJsonValue(jCam_Info));
    jRootObject.insert(msgtype,jLaneStatus);


    brtn = jSendData(jRootObject);
    if( !brtn )
    {
        QString logstr = QString("LaneStatus_Send Error(%1)").arg(centerid);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("LaneStatus_Send(%1) : ").arg(centerid);
        for(int index=0; index < imaxcam; index++)
        {
            clsCameraSTATUS camStatus = commonvalues::Remotedata->cameraST.value(index);
            logstr += QString("[%1]CAM-%2,LIC-%3,STROBE-%4").arg(index)
                    .arg(camStatus.CAMERA_STATE).arg(camStatus.LENSCONTROLBOARD_STATE)
                    .arg(camStatus.STROBE_STATE);
        }
        log->write(logstr,LOG_INFO); qDebug() << logstr;
    }

    return brtn;
}

bool CenterClient::BosuData_Send()
{
    bool brtn = false;

    QString msgtype = "BOSU_DATA";
    QString datatype = "COLLECT";
    QJsonObject jRootObject = MakeCommon(datatype,msgtype);
    QJsonObject jBosuData;
    jBosuData.insert("DEVICE_NAME",commonvalues::Remotedata->DEVICE_NAME);
    jBosuData.insert("DEVICE_INDEX",commonvalues::Remotedata->DEVICE_INDEX);
    jBosuData.insert("REPAIR_EVENT",commonvalues::Remotedata->REPAIR_EVENT);

    jRootObject.insert(msgtype,jBosuData);

    brtn = jSendData(jRootObject);
    if( !brtn )
    {
        QString logstr = QString("BosuData_Send Error(%1) : Name-%2,Index-%3 Event-%4")
                .arg(centerid).arg(commonvalues::Remotedata->DEVICE_NAME).arg(commonvalues::Remotedata->DEVICE_INDEX).arg(commonvalues::Remotedata->REPAIR_EVENT);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("BosuData_Send(%1): Name-%2,Index-%3 Event-%4")
                .arg(centerid).arg(commonvalues::Remotedata->DEVICE_NAME).arg(commonvalues::Remotedata->DEVICE_INDEX).arg(commonvalues::Remotedata->REPAIR_EVENT);
        log->write(logstr,LOG_INFO); qDebug() << logstr;
    }

    return brtn;
}

bool CenterClient::FwUpdateResult()
{
    if(commonvalues::fw_update == commonvalues::FW_UPDATE)
    {
        QString sControl_No = commonvalues::fw_controlNo;
        QString sFW_DIV = commonvalues::Remotedata->BD_NAME;
        QString sResult_DateTime = commonvalues::fw_resultTime;
        QString sYN = "적용";
        QString sFW_IMAGE_FILE_NAME = commonvalues::fw_filename;

        //펌웨어를 다운로드만 받고 적용은 안함.
        QString returndata2 = QString("%1,%2,%3,%4,%5")
                .arg(sControl_No).arg(sFW_DIV).arg(sResult_DateTime)
                .arg(sYN).arg(sFW_IMAGE_FILE_NAME);
        QString sMSG_TYPE = "FW_UPDATE_RESULT";
        CommandRESP("CONTROL",sMSG_TYPE,returndata2);
        commonvalues::fw_update = commonvalues::FW_NONE;
        emit CommandEvent(centerid,sMSG_TYPE,returndata2);
    }
}

bool CenterClient::AddCarNoData(clsHipassCarData *cardata)
{
    bool brtn = true;

    if( remoteCarDatalist.count() < MAX_REMOTEDATA )
    {
       clsHipassCarData *cardata1 = new clsHipassCarData(cardata);
       remoteCarDatalist.append(cardata1);
    }
    else
    {
        brtn = false;
        QString logstr = QString("Remote CarNoData overflow(%1) : %2").arg(centerid).arg(remoteCarDatalist.count());
        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
        plogdlg0->logappend(LogDlg::logcenter,logstr);
    }

    return brtn;
}

bool CenterClient::CarNoDataHipass_Send(clsHipassCarData *cardata)
{
    bool brtn = false;

    QString msgtype = "CAR_NO_DATA_HIPASS";
    QString datatype = "COLLECT";
    QJsonObject jRootObject = MakeCommon(datatype,msgtype);
    QJsonObject jCarNoData;
    jCarNoData.insert("TRIGGER_DATETIME",cardata->TRIGGER_DATETIME);
    jCarNoData.insert("TRIGGER_NUMBER",cardata->TRIGGER_NUMBER);
    jCarNoData.insert("TRIGGER_LANE",cardata->TRIGGER_LANE);
    jCarNoData.insert("TRIGGER_POSION",cardata->TRIGGER_POSION);
    jCarNoData.insert("SELECT_DATA",cardata->SELECT_DATA);
    jCarNoData.insert("PROCESS_RESULT",cardata->PROCESS_RESULT);
    jCarNoData.insert("PROCESS_NUMBER",cardata->PROCESS_NUMBER);
    jCarNoData.insert("PROCESS_WAY",cardata->PROCESS_WAY);
    jCarNoData.insert("WORK_NO",cardata->WORK_NO);
    jCarNoData.insert("WORK_DATE",cardata->WORK_DATE);
    jCarNoData.insert("IMAGE_NUMBER",cardata->IMAGE_NUMBER);
    jCarNoData.insert("VIO_DATETIME",cardata->VIO_DATETIME);
    jCarNoData.insert("VIO_TYPE",cardata->VIO_TYPE);
    jCarNoData.insert("VIO_CODE",cardata->VIO_CODE);
    jCarNoData.insert("RECOG_RESULT",cardata->RECOG_RESULT);
    jCarNoData.insert("RECOG_CAR_NO",cardata->RECOG_CAR_NO);
    jCarNoData.insert("RECOG_CHAR_SCORE",cardata->RECOG_CHAR_SCORE);
//    QJsonArray jCHAR_MACHING_YN;
//    QStringList ynlist = cardata->CHAR_MACHING_YN.split(",");
//    int yncount = ynlist.size();
//    for(int index=0; index < yncount; index++ )
//    {
//        jCHAR_MACHING_YN.push_back(QJsonValue(ynlist[index]));
//    }
    jCarNoData.insert("CHAR_MACHING_YN",cardata->CHAR_MACHING_YN);
    //jCarNoData.insert("CHAR_MACHING_YN",QJsonValue(jCHAR_MACHING_YN));
    jCarNoData.insert("CURR_STROBE_STATE",cardata->CURR_STROBE_STATE);
    jCarNoData.insert("CURR_ZOOM_VALUE",cardata->CURR_ZOOM_VALUE);
    jCarNoData.insert("CURR_FOCUS_VALUE",cardata->CURR_FOCUS_VALUE);
    jCarNoData.insert("CURR_IRIS_VALUE",cardata->CURR_IRIS_VALUE);
    jCarNoData.insert("CURR_CDS_VALUE",cardata->CURR_CDS_VALUE);
    jCarNoData.insert("IMAGE_FILE_NAME",cardata->IMAGE_FILE_NAME);
    jCarNoData.insert("PROCESS_ALGORITHM",cardata->PROCESS_ALGORITHM);

    jRootObject.insert(msgtype,jCarNoData);

    brtn = jSendData(jRootObject);
    if( !brtn )
    {
        QString logstr = QString("CarNoDataHipass_Send Error(%1)").arg(centerid);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("CarNoDataHipass_Send(%1): imgNum-%2, Num-%3")
                .arg(centerid).arg(cardata->IMAGE_NUMBER).arg(cardata->RECOG_CAR_NO);
        log->write(logstr,LOG_INFO); qDebug() << logstr;
    }

    return brtn;
}

bool CenterClient::CommandRESP(QString datatype,QString msg, QString data)
{
    bool brtn = false;

    QJsonObject jRootObject = MakeCommon(datatype,msg);
    QJsonObject jMsgData;

    QStringList datalist = data.split(",");

    if( msg.compare("SYSTEM_REBOOT_RESP") == 0 )
    {
        if( datalist.count() >= 2 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
        }
    }
    else if( msg.compare("CAMERA_REBOOT_RESP") == 0 )
    {
        if( datalist.count() >= 2 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
        }
    }
    else if( msg.compare("ZOOM_CONTROL_RESP") == 0 )
    {
        if( datalist.count() >= 3 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
            jMsgData.insert("ZOOM_CONTROL_RECV_VALUE",datalist[index++]);
        }
    }
    else if( msg.compare("FOCUS_CONTROL_RESP") == 0 )
    {
        if( datalist.count() >= 3 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
            jMsgData.insert("FOCUS_CONTROL_RECV_VALUE",datalist[index++]);
        }
    }
    else if( msg.compare("IRIS_CONTROL_RESP") == 0 )
    {
        if( datalist.count() >= 3 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
            jMsgData.insert("IRIS_CONTROL_RECV_VALUE",datalist[index++]);
        }
    }
    else if( msg.compare("FW_UPDATE_RESP") == 0 )
    {
        if( datalist.count() >= 2 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
        }
    }
    else if( msg.compare("FW_UPDATE_RESULT") == 0 )
    {
        if( datalist.count() >= 5 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("FW_DIV",datalist[index++]);
            jMsgData.insert("RESULT_DATETIME",datalist[index++]);
            jMsgData.insert("FW_APP_YN",datalist[index++]);
            jMsgData.insert("FW_IMAGE_FILE_NAME",datalist[index++]);
        }
    }
    else if( msg.compare("FW_INFO_RESP") == 0 )
    {
        if( datalist.count() >= 2 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
            jMsgData.insert("FW_DIV",datalist[index++]);
            jMsgData.insert("FW_VER",datalist[index++]);
        }
    }
    else if( msg.compare("OBU_MACH_UPDATE_RESP") == 0 )
    {
        if( datalist.count() >= 2 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESP_DATETIME",datalist[index++]);
        }
    }
    else if( msg.compare("OBU_MACH_UPDATE_RESULT") == 0 )
    {
        if( datalist.count() >= 4 )
        {
            int index = 0;
            jMsgData.insert("CONTROL_NO",datalist[index++]);
            jMsgData.insert("RESULT_DATETIME",datalist[index++]);
            jMsgData.insert("OBU_MACH_APP_YN",datalist[index++]);
            jMsgData.insert("OBU_MACH_FILE_NAME",datalist[index++]);
        }
    }



//    else if( msg.compare("BD_INFO_RESP") == 0 )
//    {
//        if( datalist.count() >= 2 )
//        {
//            int index = 0;
//            jMsgData.insert("CONTROL_NO",datalist[index++]);
//            jMsgData.insert("RESP_DATETIME",datalist[index++]);
//            jMsgData.insert("RESET_BOARD_INFO_S",datalist[index++]);
//        }
//    }

    jRootObject.insert(msg,jMsgData);


    brtn = jSendData(jRootObject);
    if( !brtn )
    {
        QString logstr = QString("CommandRESP Error(%1) : MSGTYPE-%2")
                .arg(centerid).arg(msg).arg(commonvalues::Remotedata->DEVICE_INDEX).arg(commonvalues::Remotedata->REPAIR_EVENT);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("CommandRESP(%1): MSGTYPE-%2")
                .arg(centerid).arg(msg);
        if(m_loglevel >= LOG_DEBUG)
        {
            logstr += QString(", DATA-%1").arg(data);
        }
        log->write(logstr,LOG_INFO); qDebug() << logstr;
    }

    return brtn;
}

bool CenterClient::FTPDownload(QString sip, QString sport, QString sid, QString spassword, QString filePath, QString fileName)
{
    bool brtn = false;
    //ftp is not NULL
    disconnectToFtp();
    //connect , login
    connectToFtp(sip,sport,sid,spassword,filePath);

    brtn = downloadFile(fileName);

    if(!brtn)
        cancelDownload();

    disconnectToFtp();
    return brtn;
}

bool CenterClient::OBUFTPDownload(QString sip, QString sport, QString sid, QString spassword, QString filePath, QString fileName)
{
    bool brtn = false;
    QString logstr;

    int mach_count = 1;
    int mach_num = 1;

    for(mach_num = 1; mach_num <= mach_count; mach_num++)
    {
        QString sFILE_NAME = QString("%1_%2").arg(fileName).arg(mach_num,3,10,QChar('0'));
        if( FTPDownload(sip,sport,sid,spassword,filePath,sFILE_NAME))
        {
            QString FTP_path =  QApplication::applicationDirPath() + "/Download";
            QString Download_File = QString("%1/%2").arg(FTP_path).arg(sFILE_NAME);

            if (!QFile::exists(Download_File))
            {
                logstr = QString("don't Exits OBU File(%1)").arg(Download_File);
                log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                break;
            }

            QFile obufile(Download_File);
            if (!obufile.open(QIODevice::ReadOnly))
            {
                logstr = QString("Cann't Open OBU File(%1)").arg(Download_File);
                log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                break;
            }

            QByteArray obuheader = obufile.read(16); //header 16byte : 2,4,2,4,4 = 16byte
            int index = 0;
            //BCD
            int ManagerID = obuheader.mid(index,2).toHex().toInt(); index += 2;
            int OBUDate = obuheader.mid(index,4).toHex().toInt(); index += 4;
            int FileNum = obuheader.mid(index,2).toHex().toInt(); index += 2;
            int OBUTotalCount = obuheader.mid(index,4).toHex().toInt(); index += 4;
            int OBUFileCount = obuheader.mid(index,4).toHex().toInt(); index += 4;
            obufile.close();

            logstr = QString("OBU Header:영업소-%1, 당일날짜-%2, 파일번호-%3, 전체건수-%4, 파일건수-%5")
                    .arg(ManagerID,4,10,QChar('0')).arg(OBUDate,8,10,QChar('0'))
                    .arg(FileNum).arg(OBUTotalCount).arg(OBUFileCount);
            log->write(logstr,LOG_NOTICE); qDebug() << logstr;

            //파일 개수 계산
            int fcount = OBUTotalCount/1000000 ;
            if( OBUTotalCount%1000000 > 0 ) fcount++;
            mach_count = fcount;
        }
        else
        {
            logstr = QString("Failed Download OBU File(%1)").arg(sFILE_NAME);
            log->write(logstr,LOG_NOTICE); qDebug() << logstr;
            break;
        }
    }

    if( mach_num > mach_count ) brtn = true;

    return brtn;
}

bool CenterClient::OBUUpdate(QString fileName)
{
    bool brtn = false;
    int mach_count = 1;
    QString FTP_path =  QApplication::applicationDirPath() + "/Download";

    while(true)
    {
        QString sFILE_NAME = QString("%1_%2").arg(fileName).arg(mach_count,3,10,QChar('0'));
        QString Download_File = QString("%1/%2").arg(FTP_path).arg(sFILE_NAME);

        if (!QFile::exists(Download_File))
        {
            break;
        }
        mach_count++;
    }

    if( mach_count <= 1) return brtn;


}


void CenterClient::connectToFtp(QString sIP, QString sPort,QString sID, QString sPW,QString sPath)
{
    emit CommandEvent(centerid,"FTPCreate",NULL);
    while(m_pftp == NULL)
    {
        msleep(1);
    }

    connect(m_pftp, SIGNAL(commandFinished(int,bool)),
            this, SLOT(ftpCommandFinished(int,bool)));

    QString ipaddr = sIP.trimmed();
    int port = sPort.toInt();
    QString useName = sID.trimmed();
    QString password = sPW.trimmed();
    QString path = sPath.trimmed();

    m_pftp->connectToHost(ipaddr,port);

    QFtp::State cur_state = m_pftp->state();
    qDebug() << "---------------------" << QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz");
    while(cur_state != QFtp::Connected )
    {
        msleep(1);
        cur_state = m_pftp->state();
    }
    qDebug() << "--------Connected ----------" << QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz");

    if (!useName.isEmpty())
        m_pftp->login(useName, password);
    else
        m_pftp->login();

    if (!path.isNull() && !path.isEmpty() )
        m_pftp->cd(path);

    QString logstr = QString("Connecting to FTP server [ip:%1,port:%2,id:%3,pw:%4,path:%5] ...")
             .arg(ipaddr).arg(sPort).arg(useName).arg(password).arg(path);
    log->write(logstr,LOG_NOTICE);
    qDebug() << logstr;

}

void CenterClient::disconnectToFtp()
{
    if (m_pftp)
    {
        m_pftp->abort();
        m_pftp->deleteLater();
        m_pftp = NULL;
        QString logstr = QString("Disconnect FTP server.");
        log->write(logstr,LOG_NOTICE);
        qDebug() << logstr;
     }

}

bool CenterClient::downloadFile(QString fileName)
{
    bool brtn = false;
    QString logstr;

    QString FTP_path =  QApplication::applicationDirPath() + "/Download";
    QDir dir(FTP_path);
    if( !dir.exists())
    {
        dir.mkdir(FTP_path);
    }

    QString Download_File = QString("%1/%2").arg(FTP_path).arg(fileName);

    if (QFile::exists(Download_File)) {           
        if(QFile::remove(Download_File))
        {
            logstr = QString("There already exists a file called [%1] in the current directory. Delete File").arg(Download_File);
            log->write(logstr,LOG_ERR);
        }
        else
        {
            logstr = QString("There already exists a file called [%1] in the current directory. Can't Delete File").arg(Download_File);
            log->write(logstr,LOG_ERR);
            return brtn;
        }
    }

    m_pftpfile = new QFile(Download_File);
    if (!m_pftpfile->open(QIODevice::WriteOnly)) {
        logstr = QString("Unable to save the file [%1]: %2.").arg(Download_File).arg(m_pftpfile->errorString());
        log->write(logstr,LOG_ERR);
        delete m_pftpfile;
        return brtn;
    }

    m_pftp->get(fileName, m_pftpfile);
    logstr = QString("Downloading %1...").arg(fileName);
    qDebug() << logstr;
    log->write(logstr,LOG_NOTICE);

    m_iFiledownloadCnt = 0;
    m_bDownload = false;
    while(m_iFiledownloadCnt < MAX_FTPDOWNLOAD_COUNT )
    {
        if(m_bDownload)
        {
            brtn = true;
            break;
        }
        m_iFiledownloadCnt++;
        msleep(100);

    };

    return brtn;
}

bool CenterClient::SendVehicleInfo(QString fename, QByteArray ftpData)
{
    if(fename.isNull() || ftpData.isNull())
        return false;
    else
    {
        if( protocol_type != CenterInfo::Remote )
        {
            return FtpSendVehicleInfo(fename,ftpData);
        }
        else
            return true;
    }
}

bool CenterClient::FtpSendVehicleInfo(QString fename, QByteArray ftpData)
{
    bool result = true;

    SendFileInfo sendFile;
                //sendFile.passDateTime = dateTime;

    if(sendFile.SaveFile(QString("%1/%2").arg(commonvalues::FTPSavePath).arg(m_centerName),fename,ftpData))
    {

        QString logstr = QString("FTP전송 파일 저장성공 : %1").arg(sendFile.filepath);
        if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logcenter,logstr);
        if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::logcenter,logstr);
        log->write(logstr, LOG_NOTICE); qDebug() << logstr;
    }
    else
    {
        QString logstr = QString("FTP전송 파일 저장실패 : %1").arg(sendFile.filepath);
        if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logcenter,logstr);
        if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::logcenter,logstr);
        log->write(logstr, LOG_NOTICE); qDebug() << logstr;
    }

    return result;
}

void CenterClient::ScanSendDataFiles()
{
    QString logstr;
    QString path = QString("%1/%2").arg(commonvalues::FTPSavePath).arg(m_centerName);
    QDir dir(path);
    try
    {
        if( !dir.exists())
        {
            dir.mkdir(path);
        }
    }
    catch( ... )
    {
        qDebug() << QString("ScanSendDataFiles-Directory Check  Exception");
        return;
    }

    QFileInfoList filepaths;
    sendFileList.ClearAll();

    try
    {
        QStringList filters;
        filters << "*.jpg" << "*.jpeg" << "*.txt";
        filepaths = dir.entryInfoList(filters);
    }
    catch( ... )
    {
        qDebug() << QString("ScanSendDataFiles-Search Entryile Exception");
        return;
    }

    foreach (QFileInfo filepath, filepaths)
    {
        SendFileInfo info;
        QString fpath = filepath.absoluteFilePath();
        if( info.ParseFilepath(fpath))
        {
            if( sendFileList.AddFile(info))
            {
                logstr = QString("FTP전송 데이터 추가 : %1").arg(fpath);
                log->write(logstr,LOG_NOTICE);  qDebug() <<  logstr;

            }
            else DeleteFile(fpath);
        }
        else
            DeleteFile(fpath);
    }

    logstr = QString("FTP전송 파일 리스트 : %1").arg(sendFileList.Count());
    log->write(logstr,LOG_NOTICE);  qDebug() <<  logstr;

}

void CenterClient::DeleteFile(QString filepath)
{
    try
    {
        QFile file(filepath);
        file.remove();
    }
    catch( ... )
    {
        qDebug() << QString("DeleteFile exception");
    }
}

void CenterClient::ftpCommandFinished(int id, bool error)
{
    QString logstr;
    if(m_pftp->currentCommand() == QFtp::ConnectToHost )
    {
        if(error)
        {
            qDebug() << m_pftp->errorString();
            disconnectToFtp();
        }

    }
    else if(m_pftp->currentCommand() == QFtp::Login )
    {
        if(error)
        {
            qDebug() << m_pftp->errorString();
        }

    }
    else if( m_pftp->currentCommand() == QFtp::Get )
    {
        if(error)
        {
            qDebug() << m_pftp->errorString();
            m_pftpfile->close();
            m_pftpfile->remove();
            m_bDownload = false;
        }
        else
        {
            m_pftpfile->close();
            m_bDownload = true;
        }
        delete m_pftpfile;
        m_pftpfile = NULL;
    }
    else if(m_pftp->currentCommand() == QFtp::Close )
    {
        if(error)
        {
            qDebug() << m_pftp->errorString();
        }

    }


}

void CenterClient::cancelDownload()
{
    m_pftp->abort();

    if(m_pftpfile != NULL )
    {
        if (m_pftpfile->exists()) {
            m_pftpfile->close();
            m_pftpfile->remove();
        }
        delete m_pftpfile;
    }
}

///////////////////// SendFileInfo  ///////////////////////////////////////

bool CenterClient::SendFileInfo::SaveFile(QString path, QString fname, QByteArray filedata)
{
    try
    {
        filename = fname;
        filepath = QString("%1/%2").arg(path).arg(fname);

        QDir dir(path);
        if (!dir.exists())
            dir.mkpath(path);

        QFile file(filepath);

        if (file.exists()) return false;

        if(file.open(QIODevice::WriteOnly))
        {
            QDataStream out(&file);
            int flen = filedata.length();
            int wflen = 0;
            while( flen > wflen )
            {
                int wlen = out.writeRawData(filedata.data(),flen);
                if(wlen <= 0)
                    break;
                wflen += wlen;
            }
            //            out << filedata;
             file.close();

            if(flen > wflen)
                return false;
        }
        else
            return false;
    }
    catch ( ... )
    {
        qDebug() << QString("SaveFile Expection : %1/%2").arg(path).arg(fname);
        return false;
    }
    return true;
}

bool CenterClient::SendFileInfo::ParseFilepath(QString _filepath)
{
    try
    {
        filename = _filepath.mid(_filepath.lastIndexOf('/') + 1);
//        QString filename2 = filename.mid(0, filename.lastIndexOf(".jpg"));

//        int index = filename2.indexOf("H");
//        if (index != 0)
//        {
//            index = filename2.indexOf("MS");
//            if (index != 0)
//                    return false;
//        }
        filepath = _filepath;


    }
    catch (...)
    {
        qDebug() << QString("ParseFilepath Expection : %1").arg(filepath);
        return false;
    }
    return true;
}


bool CenterClient::SendFileInfoList::AddFile(CenterClient::SendFileInfo data)
{
    bool brtn = true;
    mutex.lock();
    try
    {
        if( fileList.count() >= MAX_FILE)
        {
            SendFileInfo file = GetFirstFile();
            if(!file.filename.isNull() && !file.filename.isEmpty()) RemoveFirstFile(file);
        }
        fileList.append(data);
    }
    catch( ... )
    {
        qDebug() << QString("AddFile Expection");
        brtn = false;
    }
    mutex.unlock();
    return brtn;

}

CenterClient::SendFileInfo CenterClient::SendFileInfoList::GetFirstFile()
{
    SendFileInfo fileinfo;
    mutex.lock();
    if (fileList.count() > 0)
        fileinfo = fileList.first();
//    else
//        fileinfo = NULL;
    mutex.unlock();
    return fileinfo;
}

void CenterClient::SendFileInfoList::RemoveFirstFile(CenterClient::SendFileInfo data)
{
    mutex.lock();
    try
    {
        QFile file(data.filepath);
        file.remove();
        fileList.removeFirst();//   .removeOne(data);
        for(int index=0; index < fileList.count(); index++)
        {
              if( data.filepath.compare(fileList[index].filepath) == 0 )
              {
                fileList.removeAt(index);
                break;
              }

        }
    }
    catch( ... )
    {
        qDebug() << QString("RemoveFile Expection");
    }
    mutex.unlock();
}

void CenterClient::SendFileInfoList::ClearAll()
{
    mutex.lock();
    fileList.clear();
    mutex.unlock();
}

int CenterClient::SendFileInfoList::Count()
{
    return fileList.count();
}
