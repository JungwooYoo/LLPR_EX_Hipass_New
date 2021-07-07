#include "mainthread.h"
#include "commonvalues.h"
#include <QApplication>

mainthread::mainthread(CCUComm *pccu0, CCUComm *pccu1,int loglevel,QThread *parent) : QThread(parent)
{
    brun = false;
    pccucomm0 = pccu0;
    pccucomm1 = pccu1;

    log = new Syslogger(this, "mainthread", true, loglevel);
    m_loglevel = loglevel;

    codec = QTextCodec::codecForName("eucKR");
    encoderw = codec->makeEncoder( QTextCodec::IgnoreHeader);
}

mainthread::~mainthread()
{
    log->deleteLater();
}

void mainthread::SetLogLevel(int loglevel)
{
    log->m_loglevel = loglevel;
    m_loglevel = loglevel;
}

bool mainthread::start()
{
    QThread::start();
    brun = true;

    return brun;
}

void mainthread::run()
{
    QString logstr;
    int checkcount = 0;
    m_isignalflag  = 0xFFFF;

    logstr = QString("mainthread start");
    log->write(logstr,LOG_ERR); qDebug() << logstr;

    while(brun)
    {
        try
        {
            if( checkcount%2000 == 0) //10sec
            {
                center_check();

                if( (m_isignalflag & 0xFFFF) == 0)
                {
                    logstr = QString("No signal Error!!");
                    log->write(logstr,LOG_ERR); qDebug() << logstr;
                    msleep(500);
                    QApplication::exit();
                }
                else
                    m_isignalflag = 0;
            }

            checkcount++;
        }
        catch( ... )
        {
            logstr = QString("--------- Error center_check -----------");
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }


        try
        {
         //recog result process
                if( !procindex.isEmpty()) //check recog command
                {
                    VehcileIndex vehicle = procindex.takeFirst();
                    int index = vehicle.index;
                    int channel = vehicle.channel;
                    //Make FTP DATA, Save DB
                    createjpegimage(channel, index);
                    //중복제거
                    int recount = ViolationImageList[channel].removeAll(index);
                    if( recount > 0 )
                    {
                        QString logstr = QString("Remove Overlap violoationImage(index:%1) : %2").arg(index).arg(recount);
                        log->write(logstr,LOG_INFO); qDebug() << logstr;
                    }

                    ViolationImageList[channel].append(index);
                    QString logstr =  QString("ViolationImageList append: imagecount-%1/(%2)")
                               .arg(ViolationImageList[channel].count())
                                .arg(commonvalues::vehicledatalist[channel][index].recogtime,0,'f',2);
                    log->write(logstr); qDebug() << logstr;

                    //DB Save
                    pdatabase->updatedata(channel,index);
                }
        }
        catch( ... )
        {
            logstr = QString("--------- Error createjpegimage/DB Save -----------");
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }

        try
        {
            //위반확인응답 과 차량통보 처리
            //FTP transfer
            for(int ch=0; ch < commonvalues::cameraChannel; ch++)
            {
                VioRepCheck(ch);
            }
            //신프로토콜, 확정영상,차량통보
            if(commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && commonvalues::cameraChannel > 1)
            {
                VehicleNotification_NEW();
            }
        }
        catch( ... )
        {
            logstr = QString("--------- Error VioRepCheck/VehicleNotification_NEW -----------");
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }
        msleep(5);
    }

    logstr = QString("mainthread end");
    log->write(logstr,LOG_ERR); qDebug() << logstr;

}

void mainthread::stop()
{
    brun = false;
}

void mainthread::center_check()
{   
    int count = commonvalues::center_list.size();

       int client_count = commonvalues::clientlist.size();

       //check center count or create center class
       if( client_count != count)
       {
           if(client_count > count)
           {
               int index;
               for(int i=client_count; i > count; i-- )
               {
                   index = i-1;
                   if( commonvalues::clientlist.value(index)->m_clientsock >= 0 ) commonvalues::clientlist.value(index)->Disconnect();
                   commonvalues::clientlist.removeAt(index);
                   QString logstr = QString("Center Class Remove : %1").arg(i);
                   qDebug() << logstr;
                   log->write(logstr,LOG_NOTICE);
               }
           }
           else // client_count < count
           {
               for(int i=client_count; i < count; i++)
               {
                   emit createcenter(i);
                   msleep(100);

               }

           }
       }
       //check center connection  and  connect
       client_count = commonvalues::clientlist.size();
       for(int i=0; i < client_count; i++)
       {
           //check ip, port, localname

           if( commonvalues::center_list.value(i).ip.compare(commonvalues::clientlist.value(i)->m_ip) != 0
                   || commonvalues::center_list[i].tcpport != (int)commonvalues::clientlist.value(i)->m_port
       || commonvalues::center_list[i].centername.compare(commonvalues::clientlist.value(i)->m_centerName) != 0
                   || commonvalues::center_list[i].protocol_type != commonvalues::clientlist.value(i)->protocol_type)
           {
               if(commonvalues::clientlist.value(i)->m_clientsock >= 0)
               {
                   commonvalues::clientlist.value(i)->Disconnect();
                   commonvalues::center_list[i].status = false;
                   qDebug() << "Center Disconnect : " << i;
               }
           }

           if( commonvalues::center_list.value(i).fileNameSelect != commonvalues::clientlist.value(i)->fileNameSelect)
           {
               commonvalues::clientlist.value(i)->fileNameSelect = commonvalues::center_list.value(i).fileNameSelect;
           }


           if( commonvalues::clientlist.value(i)->m_clientsock < 0 )
           {
               emit connectcenter(i);

               msleep(10);

           }

           //client_count = commonvalues::clientlist.size();
       }

       // center connection check

       for(int i=0; i < client_count; i++)
       {
           if( commonvalues::clientlist.value(i)->m_clientsock >= 0)
           {
               commonvalues::center_list[i].status = true;

           }
           else
           {
               commonvalues::center_list[i].status = false;
           }
       }
}


void mainthread::createjpegimage(int channel, int index)
{
    QImage imag;
    QImage plateimag;
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << QString("createjpegimage Start(ch%1) : %2").arg(channel).arg(index);
    //whole image -> jpeg    
    if(commonvalues::vehicledatalist[channel][index].imglen > 0)
    {
        imag = QImage(commonvalues::vehicledatalist[channel][index].img,commonvalues::cameraSys[channel].cam_image_width,commonvalues::cameraSys[channel].cam_image_height,QImage::Format_RGB888);
        QByteArray imagdata;
        QBuffer buffer(&imagdata);
        imag.save(&buffer,"JPG");
        commonvalues::vehicledatalist[channel][index].saveimglen = imagdata.size();
        memcpy(commonvalues::vehicledatalist[channel][index].saveimg,imagdata.constData(),commonvalues::vehicledatalist[channel][index].saveimglen);
    }
    else commonvalues::vehicledatalist[channel][index].saveimglen=0;

    if( commonvalues::vehicledatalist[channel][index].recogresult > 0)
    {
        plateimag = QImage(commonvalues::vehicledatalist[channel][index].plateImage,commonvalues::vehicledatalist[channel][index].plate_width,commonvalues::vehicledatalist[channel][index].plate_height,QImage::Format_RGB888);
        QByteArray plateimagdata;
        QBuffer platebuffer(&plateimagdata);
        plateimag.save(&platebuffer,"JPG");
        commonvalues::vehicledatalist[channel][index].saveplateimglen = plateimagdata.size();
        memcpy(commonvalues::vehicledatalist[channel][index].saveplateimg,plateimagdata.constData(),commonvalues::vehicledatalist[channel][index].saveplateimglen);
    }
    else
    {
        commonvalues::vehicledatalist[channel][index].saveplateimglen = 0;
        //commonvalues::vehicledatalist[channel][index].saveplateimg = NULL;
    }
}

void mainthread::VioRepCheck(int channel)
{
    QString logstr;
    try
    {
        CCUComm *pccuComm;
        LogDlg *plogdlg;
        if(channel == 1)
        {
            pccuComm = pccucomm1;
            plogdlg = plogdlg1;
        }
        else
        {
            pccuComm = pccucomm0;
            plogdlg = plogdlg0;
        }

        int ccucount = commonvalues::ccuViolationList[channel].count();
        if( ccucount > 0)
        {
            for(int cindex = 0;  cindex < ccucount; cindex++)
            {
                //msleep(100); -> ccu에 Sleep 추가로 필요없음
                ViolationInfo violationinf = commonvalues::ccuViolationList[channel][cindex];

                int vioresultindex = -1;
                bool bremovelist = false;

//                if( LOG_DEBUG <= m_loglevel )
//                {
//                    logstr = QString("ViolationImageList Count(ch%1) : %2").arg(channel).arg(ViolationImageList[channel].count());
//                    //log->write(logstr,LOG_DEBUG);
//                    qDebug() << logstr;
//                }


                foreach (int vioimgIndex, ViolationImageList[channel])
                {
//                    if( LOG_DEBUG <= m_loglevel )
//                    {
//                        logstr = QString("ViolationImageList index(ch%1) : %2").arg(channel).arg(vioimgIndex);
//                        //log->write(logstr,LOG_DEBUG);
//                        qDebug() << logstr;
//                    }

                    //itronics
                    if(commonvalues::ccutype == commonvalues::CCU_ITRONICS && (quint16)commonvalues::vehicledatalist[channel][vioimgIndex].ccu_seq == violationinf.seq)
                    {
                        vioresultindex = vioimgIndex;
                        break;
                    }
                    //nomarl
                    else if(commonvalues::ccutype == commonvalues::CCU_NORMAL && (quint16)commonvalues::vehicledatalist[channel][vioimgIndex].car_num == violationinf.violationIndex)
                    {
                        vioresultindex = vioimgIndex;
                        break;
                    }
                    //daebo
                    else if(commonvalues::ccutype == commonvalues::CCU_DAEBO && (quint16)commonvalues::vehicledatalist[channel][vioimgIndex].ccu_seq == violationinf.seq)
                    {
                        vioresultindex = vioimgIndex;
                        break;
                    }
                    else if(commonvalues::ccutype == commonvalues::CCU_sTRAFFIC && (quint16)commonvalues::vehicledatalist[channel][vioimgIndex].ccu_seq == violationinf.seq)
                    {
                        vioresultindex = vioimgIndex;
                        break;
                    }
                    else if(commonvalues::ccutype == commonvalues::CCU_TCS_SDS && (quint16)commonvalues::vehicledatalist[channel][vioimgIndex].car_num == violationinf.violationIndex)
                    {
                        vioresultindex = vioimgIndex;
                        break;
                    }
                }

                //위반확인응답의 시간에서 로컬시간을 사용 하도록 수정함.
                QDateTime dviotime = QDateTime::fromString(QString((const char *)violationinf.recvTime),"yyyyMMddHHmmss"); //violationinf.recvTime;   //QDateTime::fromString(violationinf.recvTime,"yyyyMMddHHmmss");
                qint64 diffsec = dviotime.secsTo(QDateTime::currentDateTime());


                if(vioresultindex >= 0)
                {
                    if( LOG_DEBUG <= m_loglevel )
                    {
                        logstr = QString("ViolationImageList Count(ch%1) : %2").arg(channel).arg(ViolationImageList[channel].count());
                        log->write(logstr,LOG_DEBUG); qDebug() << logstr;
                    }

                    //확정영상을 처리하기 위해서 FTP를 먼저 생성 후에 차량번호통보를 전송한다.
                    //FTP Send
                    FTPSend(channel,vioresultindex,violationinf);
                    //vehicle notification
                    //전면만 사용함.
                    if(!commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification
                            && commonvalues::cameraChannel == 1) //위반응답이 여러번와도 한번만 차량통보를 함.
                    {
                       // pccuComm->Vehicle_Notification(pccuComm->m_sequenceindex, violationinf.violationIndex
                        quint8 rseq;
                        if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
                        {
                            rseq = violationinf.seq;
                        }
                        else //commonvalues::One_Vehicle_Two_Seq
                        {
                            if( violationinf.seq == 0xFF ) rseq = 0x01;
                            else rseq = violationinf.seq + 1;
                        }

                        pccuComm->Vehicle_Notification( rseq , violationinf.violationIndex
                                                       , QString((const char *)commonvalues::vehicledatalist[channel][vioresultindex].recognum));
                        commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification = true;

//                        logstr = QString("차량번호통보전송(ch%1) : seq-%2, index-%3, num-%4")
//                                .arg(channel).arg(rseq).arg(violationinf.violationIndex).arg(QString((const char *)commonvalues::vehicledatalist[channel][vioresultindex].recognum));
//                        plogdlg->logappend(LogDlg::logccu,logstr);
//                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;

//                        if(pccuComm->m_sequenceindex == 0xFF) pccuComm->m_sequenceindex = 1;
//                        else pccuComm->m_sequenceindex++;
                    }
                    //전후면 사용시
                    else if(!commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification
                            && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && commonvalues::cameraChannel > 1)
                    { //bVehicleNotification는 각 채널에 대한 통보로 2채널 각각이 전송한다는것을 전제로함.
                        commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification = true;
                    }
                    //전후면 사용 , Two_CCU 또는 NoConfirm
                    else if( !commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification
                             && ( commonvalues::confirmSysType == commonvalues::NO_Confirm || commonvalues::confirmSysType == commonvalues::CCU_Two_Confirm )
                             && commonvalues::cameraChannel > 1  )
                    {
                        quint8 rseq;
                        if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
                        {
                            rseq = violationinf.seq;
                        }
                        else //commonvalues::One_Vehicle_Two_Seq
                        {
                            if( violationinf.seq == 0xFF ) rseq = 0x01;
                            else rseq = violationinf.seq + 1;
                        }

                        pccuComm->Vehicle_Notification( rseq , violationinf.violationIndex
                                                       , QString((const char *)commonvalues::vehicledatalist[channel][vioresultindex].recognum));
                        commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification = true;

//                        logstr = QString("차량번호통보전송(ch%1) : seq-%2, index-%3, num-%4")
//                                .arg(channel).arg(rseq).arg(violationinf.violationIndex).arg(QString((const char *)commonvalues::vehicledatalist[channel][vioresultindex].recognum));
//                        plogdlg->logappend(LogDlg::logccu,logstr);
//                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                    }
                    bremovelist = true;
                }
                //위반응답이 먼저 들어온 경우 삭제를 안하기 위해 추가됨.
                else if( diffsec > commonvalues::cameraSys[channel].ccu_viorepTimout)//ccu 1개 전후면 사용을 위해서
                {
                    //인식이 늦으면 미처리로 갈 수 있음.
                    //인식결과가 3개 이상이 쌓일 때 미처리도 동작함.
                    logstr = QString("위반확인응답 미처리 데이터 삭제(ch%1)(seq:%2,vionum:%3)").arg(channel).arg(violationinf.seq).arg(violationinf.violationIndex);
                    log->write(logstr,LOG_DEBUG); qDebug() << logstr;

                    bremovelist = true;
                }

                if( bremovelist)
                {
                    for(int index=0; index < commonvalues::ccuViolationList[channel].count(); index++)
                    {
                          if( violationinf.seq == commonvalues::ccuViolationList[channel][index].seq
                                  && violationinf.violationIndex == commonvalues::ccuViolationList[channel][index].violationIndex )
                          {
                            commonvalues::ccuViolationList[channel].removeAt(index);
                            break;
                          }

                    }

                    logstr = QString("위반확인응답 리스트(ch%1) : %2").arg(channel).arg(commonvalues::ccuViolationList[channel].count());
                    log->write(logstr,LOG_DEBUG); qDebug() << logstr;
                    break;
                }
            }

        }

        //적재된 사용 안하는 데이터 삭제
        ccucount = commonvalues::ccuViolationList[channel].count();
        if( ccucount > 0)
        {
            QList<int> removelist;
            for(int cindex = 0;  cindex < ccucount; cindex++)
            {
                ViolationInfo violationinf = commonvalues::ccuViolationList[channel][cindex];
                QDateTime vioTime = QDateTime::fromString(QString((const char *)violationinf.recvTime),"yyyyMMddHHmmss");
                qint64 timeout = vioTime.secsTo(QDateTime::currentDateTime());
                //타임아웃, 10초 이상의 데이터삭제
                int oldindex = ccucount - cindex;
                if( oldindex > 10)
                {
                    removelist.append(cindex);
                }
                else if( timeout > 10 )//10sec timeout
                {
                    removelist.append(cindex);
                }

            }

            while(removelist.count() > 0)
            {
                int rindex = removelist.last();

                ViolationInfo violationinf = commonvalues::ccuViolationList[channel][rindex];
                logstr = QString("위반응답타임아웃&Overflow(ch%1) : seq-%2, imgNum-%3, recvTime-%4")
                        .arg(channel).arg((quint8)violationinf.seq).arg((quint16)violationinf.violationIndex)
                        .arg(QString((const char *)violationinf.recvTime));
                log->write(logstr,LOG_INFO); qDebug() << logstr;

                commonvalues::ccuViolationList[channel].removeAt(rindex);
                removelist.removeLast();
            }
        }

        //이미지는 최대 8장까지 보관한다.
        //miss매칭 영상처리(응답이 일정시간(5초)동안 안들어오는 경우 동작함.)
        if( ViolationImageList[channel].count() > 0 )
        {
            int timeout_vioresultindex = -1;
            foreach (int vioimgIndex, ViolationImageList[channel])
            {
                QDateTime dviotime = QDateTime::fromString(QString((const char *)commonvalues::vehicledatalist[channel][vioimgIndex].car_entrytime),"yyyyMMdd-HHmmss");
                int diffsec = (int)dviotime.secsTo(QDateTime::currentDateTime());
                if( diffsec > commonvalues::cameraSys[channel].ccu_viorepTimout)
                {
                    //miss매칭 타임아웃 시간보다 오래된 영상은 리스트에서 삭제
                    //miss매칭영상 전송 , 한번이라도 보낸 인식결과는 미처리영상으로 안보냄.
                    if (!commonvalues::vehicledatalist[channel][vioimgIndex].bVehicleNotification)
                    {
                        ViolationInfo violationinf_NULL;
                        FTPSend(channel,vioimgIndex,violationinf_NULL, true);
                        commonvalues::vehicledatalist[channel][vioimgIndex].bVehicleNotification = true;
                        timeout_vioresultindex = vioimgIndex;
                        break;
                    }
                }
            }

            if( timeout_vioresultindex >= 0 )
            {
                //miss매칭영상 전송 , 한번이라도 보낸 인식결과는 미처리영상으로 안보냄.
                for(int index=0; index < ViolationImageList[channel].count(); index++)
                {
                    int vioresultindex = ViolationImageList[channel][index];
                    logstr = QString("D)ViolationImageList imagecount(ch%1) : %2/%3, index-%4").arg(channel)
                            .arg(commonvalues::vehicledatalist[channel][vioresultindex].seq).arg(commonvalues::vehicledatalist[channel][vioresultindex].recogtime,0,'f',2).arg(index);
                    log->write(logstr,LOG_DEBUG); qDebug() << logstr;
                }
                logstr = QString("Remove ViolationImageList imagecount(ch%1) : %2/%3").arg(channel)
                        .arg(commonvalues::vehicledatalist[channel][timeout_vioresultindex].seq).arg(ViolationImageList[channel].count());
                log->write(logstr,LOG_DEBUG); qDebug() << logstr;

                ViolationImageList[channel].removeOne(timeout_vioresultindex);

//                foreach (int vioresultindex, ViolationImageList[channel]) {
//                    logstr = QString("ViolationImageList imagecount(ch%1) : %2/%3").arg(channel)
//                            .arg(commonvalues::vehicledatalist[channel][vioresultindex].seq).arg(commonvalues::vehicledatalist[channel][vioresultindex].recogtime,0,'f',2);
//                    log->write(logstr,LOG_DEBUG); qDebug() << logstr;
//                }
            }
            //인식결과 8장 보관,  그 이상 저장 시 처음 저장된 영상 삭제
            if ( ViolationImageList[channel].count() > 8)
            {
                int vioresultindex = ViolationImageList[channel].first();

                //miss매칭 영상큐의 수 제한으로 인한 미처리 영상 전송
                //miss매칭영상 전송 , 한번이라도 보낸 인식결과는 미처리영상으로 안보냄.
                if (!commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification)
                {
                    ViolationInfo violationinf_NULL;
                    FTPSend(channel,vioresultindex,violationinf_NULL, true);
                    commonvalues::vehicledatalist[channel][vioresultindex].bVehicleNotification = true;
                }

                logstr = QString("인식결과 리스트 Overflow(ch%1) : %2(seq-%3,imgNum-%4)").arg(channel)
                        .arg(ViolationImageList[channel].count())
                        .arg(commonvalues::vehicledatalist[channel][vioresultindex].ccu_seq)
                        .arg(commonvalues::vehicledatalist[channel][vioresultindex].car_num);
                log->write(logstr,LOG_INFO); qDebug() << logstr;

                ViolationImageList[channel].removeFirst();

            }

        }
    }
    catch( ... )
    {
        logstr = QString("VioRepCheck Error");
        log->write(logstr,LOG_ERR);
    }
}

void mainthread::FTPSend(int channel, int vioresultIndex, ViolationInfo violationinf, bool bmissmatch)
{
    try
    {
        //OBU matching
        //commonvalues::vehicledatalist[channel][vioresultIndex].recognum
        QByteArray obumatch;
        if(!bmissmatch && commonvalues::obuMatch > 0)
        {
            QString orgvehiclenum = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);
            //차량번호수정
            obumatch = OBUMatch(channel, vioresultIndex,  violationinf);
            if(obumatch.count(commonvalues::OBU_MisMatch) > 0)
            {
                QString modvehiclenum = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);
                //차량번호수정
                QString logstr = QString("OBUMatch: %1 -> %2").arg(orgvehiclenum).arg(modvehiclenum);
                if(channel == 0)
                {
                    if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logccu,logstr); qDebug() << logstr;
                }
                else
                {
                    if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::logccu,logstr); qDebug() << logstr;
                }
                log->write(logstr,LOG_INFO);

            }
        }
        else
        {
            int len = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum).size();
            obumatch = QByteArray(len,(char)commonvalues::OBU_NoMatch);
        }


        QString FtpFilename1;
        QString FtpFilename2;
        quint32 processNum = 0;
        QByteArray ftpdata = MakeFTPFile(channel,vioresultIndex, violationinf,bmissmatch,&FtpFilename1,&FtpFilename2,&processNum);

        //----> center send
        int client_count = commonvalues::clientlist.size();
        for(int index=0; index < client_count; index++)
        {
            //front
            if(commonvalues::clientlist[index]->fileNameSelect == CenterInfo::H_Char
                    && commonvalues::cameraChannel > 1)
            { /*전후면 확정영상만사용  H~~~.jpg*/
                if(bmissmatch)
                { //미처리영상
                    commonvalues::clientlist[index]->SendVehicleInfo(FtpFilename2,ftpdata);
                }
            }
            else //전후면영상전송, 1채널 시 영상전송
            {
                if(commonvalues::cameraChannel > 1)  //F.jpg, R.jpg
                    commonvalues::clientlist[index]->SendVehicleInfo(FtpFilename2,ftpdata);
                else                    //H~~~.jpg
                    commonvalues::clientlist[index]->SendVehicleInfo(FtpFilename1,ftpdata);
            }

        }

        //원격통신데이터생성
        clsHipassCarData *cardata = new clsHipassCarData();
        if(!bmissmatch)
        {
            cardata->TRIGGER_DATETIME = QString("%1%2").arg(QString((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].car_entrytime).remove('-')).arg("000"); //"yyyyMMdd-HHmmss"
            cardata->TRIGGER_NUMBER = "0";
            cardata->TRIGGER_LANE = "1";
            cardata->TRIGGER_POSION = channel == 0 ? "전면" : "후면";
            cardata->SELECT_DATA = "확정";
            cardata->PROCESS_RESULT = "정상";
            cardata->PROCESS_NUMBER = QString::number(violationinf.processNumber);
            cardata->PROCESS_WAY = "동영상";
            cardata->WORK_NO = QString("%1%2").arg(violationinf.workNumber1,2,16,QChar('0')).arg(violationinf.workNumber2,2,16,QChar('0'));
            cardata->WORK_DATE = QString((const char *)violationinf.workDate);
            cardata->IMAGE_NUMBER = QString::number(violationinf.violationIndex);
            cardata->VIO_DATETIME = QString((const char *)violationinf.violationTime);
            cardata->VIO_TYPE = QString::number(violationinf.violationType);
            cardata->VIO_CODE = QString::number(violationinf.violationCode);

            QString recogcarnum = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);
            if( recogcarnum.contains("xxxxxx"))
            {
                cardata->RECOG_RESULT = "인식실패" ;
            }
            else if(recogcarnum.contains("x"))
            {
                cardata->RECOG_RESULT = "개별인식" ;
            }
            else
            {
                cardata->RECOG_RESULT = "정인식" ;
            }
            cardata->RECOG_CAR_NO = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);            
            int obumatchcount = obumatch.size();
            QString strmatching_yn;
            QString strchar_score;
            for(int mindex=0; mindex < obumatchcount; mindex++)
            {
                if(mindex == 0)
                {
                    if(obumatch.at(mindex) == commonvalues::OBU_MisMatch)
                        strmatching_yn = "Y";
                    else
                        strmatching_yn = "N";

                    strchar_score = "90";
                }
                else
                {
                    if(obumatch.at(mindex) == commonvalues::OBU_MisMatch)
                        strmatching_yn += ",Y";
                    else
                        strmatching_yn += ",N";

                    strchar_score += ",90";
                }
            }
            cardata->RECOG_CHAR_SCORE = strchar_score;
            cardata->CHAR_MACHING_YN = strmatching_yn;
            cardata->CURR_STROBE_STATE = "켜짐";
            cardata->CURR_ZOOM_VALUE = QString::number(commonvalues::currentvalue[channel].cur_zoom);
            cardata->CURR_FOCUS_VALUE = QString::number(commonvalues::currentvalue[channel].cur_focus);
            cardata->CURR_IRIS_VALUE = QString::number(commonvalues::currentvalue[channel].cur_iris);
            cardata->CURR_CDS_VALUE = QString::number(commonvalues::currentvalue[channel].cur_cds);
            cardata->IMAGE_FILE_NAME = QString(FtpFilename1.data(),FtpFilename1.size());
            int matching_y = strmatching_yn.count("Y");
            int matching_n = strmatching_yn.count("N");
            if( matching_y > 0 && matching_n > 0)
                cardata->PROCESS_ALGORITHM = "2";
            else if( matching_y > 0)
                cardata->PROCESS_ALGORITHM = "1";
            else
                cardata->PROCESS_ALGORITHM = "0";
        }


        if(!bmissmatch && commonvalues::cameraChannel > 1)
        {

            InsertFTPData(violationinf.seq,violationinf.violationIndex,channel
                              ,QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum)
                              ,FtpFilename1, FtpFilename2, ftpdata,cardata);

        }

        qDebug() << QString("###############clsFTP check(ch%1): %2, %3, %4, %5").arg(channel)
                    .arg(commonvalues::cameraChannel).arg(processNum).arg(ftpdata.length()).arg(bmissmatch ? "TRUE" : "FALSE");

        //원격서버에 처리 차량번호 데이터전송 //차량통보 시에 사용함 //1채널 시 전송
        if(!bmissmatch)
        {
            if(!commonvalues::vehicledatalist[channel][vioresultIndex].bVehicleNotification
                    && commonvalues::cameraChannel == 1)
            {
                int client_count = commonvalues::clientlist.size();
                for(int index=0; index < client_count; index++)
                {

                   if(commonvalues::clientlist[index]->protocol_type == CenterInfo::Remote)  //H~~~.jpg
                   {
                       commonvalues::clientlist[index]->AddCarNoData(cardata);
                   }
                }
            }
        }

        if( cardata != NULL )
        {
            delete cardata;
            cardata = NULL;
        }

    }
    catch( ... )
    {
        QString logstr = QString("FTP Send Error(ch%1, index : %2)").arg(channel).arg(vioresultIndex);
        log->write(logstr,LOG_ERR); qDebug() << logstr;
    }
}

QByteArray mainthread::MakeFTPFile(int channel, int vioresultIndex, ViolationInfo violationinf, bool bmissmatch,QString *filename1, QString *filename2,quint32 *processNum)
{
    QByteArray ftpdata;
    try
    {
        uint datalen = 0;

        if(commonvalues::vehicledatalist[channel][vioresultIndex].saveimglen > 0)
        {
            ftpdata.append(QByteArray((const char *)commonvalues::vehicledatalist[channel][vioresultIndex].saveimg,(int)commonvalues::vehicledatalist[channel][vioresultIndex].saveimglen));
            datalen += commonvalues::vehicledatalist[channel][vioresultIndex].saveimglen;
        }
        if(commonvalues::vehicledatalist[channel][vioresultIndex].saveplateimglen > 0)
        {
            ftpdata.append(QByteArray((const char *)commonvalues::vehicledatalist[channel][vioresultIndex].saveplateimg,(int)commonvalues::vehicledatalist[channel][vioresultIndex].saveplateimglen));
            datalen += commonvalues::vehicledatalist[channel][vioresultIndex].saveplateimglen;
        }
        //513bye
        QByteArray sysState_Type("RUN NT "); // 4 + 3 = 7
        QByteArray sysVersion(4,0x00);  sysVersion[0] = 0x01;
        QByteArray RecogState(4,0x00);  RecogState[0] = commonvalues::vehicledatalist[channel][vioresultIndex].recogresult > 0 ? 0x02 : 0x01;
        QByteArray LaneID  = Uint32toArr_Little(commonvalues::LaneID);
        QByteArray ManagerID = Uint32toArr_Little(commonvalues::ManagerID);
        QByteArray LaneDirection = commonvalues::LandDirection.toUtf8();
        //reserved1  4  -> index 7 + 4 + 4 + 4 + 4 + 1 = 24
        QByteArray DiscountCardVersion("DISC"); //4byte

        QByteArray VioConfirmData = bmissmatch ? QByteArray(82,0x00) : QByteArray( MakeViolationData(violationinf)); //82bytes
        //reserved2   3   18bytes   ->  index : 114
        //reserved3   15
        QByteArray FullImageSize = Uint32toArr_Little(commonvalues::vehicledatalist[channel][vioresultIndex].saveimglen);
        QByteArray PlateImageSize = Uint32toArr_Little(commonvalues::vehicledatalist[channel][vioresultIndex].saveplateimglen);
        //reserved4   10bytes  -> index : 140
        QString strvehicle = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);
        //"영"체크 후 삭제 문자열이 12자를 넘음
        if( strvehicle.contains("영"))
        {
            QString prestr = strvehicle.mid(0);
            strvehicle.remove("영");
            QString logstr = QString("ftp header - 영 제거: %1->%2").arg(prestr).arg(strvehicle);
            qDebug() << logstr; log->write(logstr,LOG_INFO);
        }
        QByteArray bRecogResult = encoderw->fromUnicode(strvehicle ); //12byte 체크 필요함.
        int restlen = 12 - bRecogResult.length();
        if( restlen > 0 ) bRecogResult.append(QByteArray(restlen,0x00));
        QByteArray PlateXStart = Uint32toArr_Little((uint)commonvalues::vehicledatalist[channel][vioresultIndex].plate_x);
        QByteArray PlateYStart = Uint32toArr_Little((uint)commonvalues::vehicledatalist[channel][vioresultIndex].plate_y);
        int xend = commonvalues::vehicledatalist[channel][vioresultIndex].plate_x + commonvalues::vehicledatalist[channel][vioresultIndex].plate_width;
        QByteArray PlateXEnd = Uint32toArr_Little((uint)xend);
        int yend = commonvalues::vehicledatalist[channel][vioresultIndex].plate_y + commonvalues::vehicledatalist[channel][vioresultIndex].plate_height;
        QByteArray PlateYEnd = Uint32toArr_Little((uint)yend);
        QByteArray RawImageWidth = Uint32toArr_Little(commonvalues::cameraSys[channel].cam_image_width);
        QByteArray RawImageHeight = Uint32toArr_Little(commonvalues::cameraSys[channel].cam_image_height);
        QByteArray RawImageSize = Uint32toArr_Little(commonvalues::vehicledatalist[channel][vioresultIndex].rawImagelen);
        QByteArray CameraType("GS2");
        QByteArray bEOF("EOF");

        QByteArray headerdata;
        headerdata.append(sysState_Type);
        headerdata.append(sysVersion);
        headerdata.append(RecogState);
        headerdata.append(LaneID);
        headerdata.append(ManagerID);
        headerdata.append(LaneDirection);
        headerdata.append(QByteArray(4,0x00)); //reserved1  4  -> index 7 + 4 + 4 + 4 + 4 + 1 = 24
        headerdata.append(DiscountCardVersion);
        headerdata.append(VioConfirmData);
        headerdata.append(QByteArray(3,0x00));  //reserved2   3   18bytes   ->  index : 114
        headerdata.append(QByteArray(15,0x00)); //reserved3   15
        headerdata.append(FullImageSize);
        headerdata.append(PlateImageSize);
        headerdata.append(QByteArray(10,0x00)); //reserved4   10bytes  -> index : 140
        headerdata.append(bRecogResult);
        headerdata.append(PlateXStart);
        headerdata.append(PlateYStart);
        headerdata.append(PlateXEnd);
        headerdata.append(PlateYEnd);
        headerdata.append(RawImageWidth);
        headerdata.append(RawImageHeight);
        headerdata.append(RawImageSize);
        headerdata.append(CameraType);
        headerdata.append(QByteArray(317,0x00));
        headerdata.append(bEOF);

        ftpdata.append(headerdata);

        //if(commonvalues::cameraChannel < 2)
        //전면파일이름, 1채널용
        {
            if( bmissmatch)
            {
                *filename1 = QString("MS%1%2%3.jpg")
                        .arg(commonvalues::LandDirection).arg(QString((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].car_entrytime).remove('-'))
                        .arg(commonvalues::cameraSys[channel].MSindex,4,10,QChar('0'));
                if( commonvalues::cameraSys[channel].MSindex >= 9999) commonvalues::cameraSys[channel].MSindex = 1;
                else commonvalues::cameraSys[channel].MSindex++;
            }
            else
            {
                if(violationinf.violationCode == 0xFF)
                {
                    *filename1 = QString("H%1%2%3%4%5.jpg")
                            .arg(QString((const char *)violationinf.violationTime)).arg(QString((const char *)violationinf.workDate))
                            .arg(violationinf.workNumber1,2,10,QChar('0')).arg(violationinf.workNumber2,2,10,QChar('0'))
                            .arg(violationinf.processNumber,8,16,QChar('0'));
                }
                else
                {
                    *filename1 = QString("H%1%2%3%4.jpg")
                            .arg(QString((const char *)violationinf.workDate)).arg(violationinf.workNumber1,2,10,QChar('0'))
                            .arg(violationinf.workNumber2,2,10,QChar('0')).arg(violationinf.processNumber,8,16,QChar('0'));
                }

                *processNum = violationinf.processNumber;

            }

        }
        //else  //
        //전후면파일이름
        {
            if( bmissmatch )
            {
                *filename2 = QString("MS%1%2%3.jpg")
                        .arg(commonvalues::LandDirection).arg(QString((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].car_entrytime).remove('-'))
                        .arg(commonvalues::cameraSys[channel].MSindex,4,10,QChar('0'));
                if( commonvalues::cameraSys[channel].MSindex >= 9999) commonvalues::cameraSys[channel].MSindex = 1;
                else commonvalues::cameraSys[channel].MSindex++;

            }
            else
            {  //filename -> X~~~~?
                *filename2 = QString("X%1%2%3%4%5%6.jpg")
                        .arg(commonvalues::ManagerID,3,10,QChar('0')).arg(QString((const char *)violationinf.workDate))
                        .arg(violationinf.workNumber1,2,10,QChar('0')).arg(violationinf.workNumber2,2,10,QChar('0'))
                        .arg(violationinf.processNumber,8,16,QChar('0')).arg(channel == 0 ? "F" : "R");

                //*processNum = violationinf.processNumber;

            }

        }

    }
    catch( ... )
    {
        QString logstr = QString("MakeFTPFile exception(ch%1) : index(%2)").arg(channel).arg(vioresultIndex);

        qDebug() << logstr;
        log->write(logstr,LOG_ERR);
        ftpdata.clear();
    }
    return ftpdata;
}

QByteArray mainthread::MakeViolationData(ViolationInfo violationinf)
{//bigendian
    QByteArray srcdata((char*)violationinf.rawdata,VIOLATION_RAW_LEN);  //41bytes
//    quint8 HworkLane = srcdata.at(12);  //BCD
//    quint8 workLane = (( HworkLane >> 4) & 0x0F ) * 10;
//           workLane += (HworkLane & 0x0F);
//    srcdata.replace(12,1,(char *)&workLane,1);

    QByteArray strhex = srcdata.toHex() ;
    //strhex.append(QByteArray(20,0x00));



    if(m_loglevel == LOG_DEBUG)
    {
        int index = 0;
        QByteArray bopcode = strhex.mid(index,2); index += 2;
        QByteArray bseq = strhex.mid(index,2); index += 2;
        QByteArray vioNumber = strhex.mid(index,4); index +=4;
        QByteArray vioYear = strhex.mid(index,4); index += 4;
        QByteArray vioMonth = strhex.mid(index,2); index += 2;
        QByteArray vioDay = strhex.mid(index,2); index += 2;
        QByteArray vioHour = strhex.mid(index,2); index += 2;
        QByteArray vioMin = strhex.mid(index,2); index += 2;
        QByteArray vioSec = strhex.mid(index,2); index += 2;
        QByteArray vioType = strhex.mid(index,2); index += 2;
        QByteArray workLane = strhex.mid(index,2); index += 2;
        QByteArray workIndexNumber = strhex.mid(index,2); index += 2;
        QByteArray workYear = strhex.mid(index,4); index += 4;
        QByteArray workMonth = strhex.mid(index,2); index += 2;
        QByteArray workDay = strhex.mid(index,2); index += 2;
        QByteArray processNumber = strhex.mid(index,8); index += 8;
        QByteArray vioCode = strhex.mid(index,2); index += 2;
        QByteArray vehicleNumber = strhex.mid(index,10); index += 10;
        QByteArray workType = strhex.mid(index,2); index += 2;
        QByteArray managerID;
        QByteArray OBUNumber;
        QByteArray OBUType;
        QByteArray PAY_OX;
        QByteArray OBU_VehicleType;
        if(srcdata.length() >= 31)
        {
            managerID = strhex.mid(index,4); index += 4;
            OBUNumber = strhex.mid(index,16); index += 16;
            OBUType = strhex.mid(index,2); index += 2;
            PAY_OX = strhex.mid(index,1); index += 1;
            OBU_VehicleType = strhex.mid(index,1); index += 1;
        }
        QString logstr = QString("MakeViolationData :%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16,%17,%18,%19,%20,%21,%22,%23,%24")
                .arg(QString(bopcode)).arg(QString(bseq)).arg(QString(vioNumber)).arg(QString(vioYear)).arg(QString(vioMonth))
                .arg(QString(vioDay)).arg(QString(vioHour)).arg(QString(vioMin)).arg(QString(vioSec)).arg(QString(vioType))
                .arg(QString(workLane)).arg(QString(workIndexNumber)).arg(QString(workYear)).arg(QString(workMonth)).arg(QString(workDay))
                .arg(QString(processNumber)).arg(QString(vioCode)).arg(QString(vehicleNumber)).arg(QString(workType)).arg(QString(managerID))
                .arg(QString(OBUNumber)).arg(QString(OBUType)).arg(QString(PAY_OX)).arg(QString(OBU_VehicleType));
        log->write(logstr,LOG_DEBUG);
        qDebug() << logstr;
    }

    return strhex;

}

QByteArray mainthread::Uint32toArr_Little(quint32 number)
{
    QByteArray conbytearr;
    conbytearr.append((quint8)(number & 0x000000FF));
    conbytearr.append((quint8)((number >> 8) & 0x000000FF));
    conbytearr.append((quint8)((number >> 16) & 0x000000FF));
    conbytearr.append((quint8)((number >> 24) & 0x000000FF));
    return conbytearr;
}

// confirmCam - front:0x01 , rear:0x02
void mainthread::ConfirmInfo(int channel,quint32 ProcessNumber,int confirmCam)
{
    int count = FTPDataList[channel].count();

    qDebug() << QString("ConfirmInfo(ch%1):%2,%3").arg(channel).arg(ProcessNumber).arg(confirmCam);
    for(int index=0; index < count ; index++)
    {
        if(ProcessNumber == FTPDataList[channel][index].processNumber )
        {
            //무조건 전면은 채널0임.
            if(channel == (confirmCam - 1) )
            {

                QString ftpfileName;
                if(channel == 1 ) ftpfileName = QString(FTPDataList[channel][index].fileName).replace("R.jpg","R.txt");
                else ftpfileName = QString(FTPDataList[channel][index].fileName).replace("F.jpg","F.txt");
                QString logstr = QString("확정영상(ch%1):%2/%3,처리번호-%4").arg(channel).arg(ftpfileName).arg( FTPDataList[channel][index].processNumber);
                log->write(logstr,LOG_NOTICE); qDebug() << logstr;

                //----> center ftp send
                int client_count = commonvalues::clientlist.size();
                for(int index=0; index < client_count; index++)
                {
                    commonvalues::clientlist[index]->SendVehicleInfo(ftpfileName,QByteArray((const char*)FTPDataList[channel][index].ftpdata));
                }
            }
            FTPDataList[channel].removeAt(index);
            break;
        }
        else
        { //확정영상은 순차적으로 처리가 된다고 생각하고 작성됨.
            FTPDataList[channel].removeAt(index);
        }
    }
}

// 0: front , rear: 1
void mainthread::InsertFTPData(quint8 seq, quint16 imgNum, quint8 front_rear, QString vehicleNumber, QString fileName1, QString fileName2, QByteArray ftpdata,clsHipassCarData *cardata)
{
    //전후 매칭데이터 확인 및 복사
    for(int index=0 ; index < MAX_CONFIRM_VEHICLE; index++)
    {
        if( commonvalues::confirmdatalist[index].seq == seq && commonvalues::confirmdatalist[index].imgNumber == imgNum )
        {

            QString logstr = QString("InsertFTPData : seq-%1,imgNum-%2,front-%3,%4,%5,%6").arg(seq).arg(imgNum).arg(front_rear)
                    .arg(vehicleNumber).arg(fileName1).arg(fileName2);
            plogdlg0->logappend(LogDlg::logccu,logstr); qDebug() << logstr;

            if(front_rear == 0)
            {//front
                strcpy((char*)commonvalues::confirmdatalist[index].frontNum,vehicleNumber.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].frontFileName1,fileName1.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].frontFileName2,fileName2.toLocal8Bit().constData());
                commonvalues::confirmdatalist[index].fFTPDatalen = ftpdata.size();
                memcpy(commonvalues::confirmdatalist[index].fFTPData,(unsigned char*)ftpdata.constData(),sizeof(unsigned char) * ftpdata.size() );
                commonvalues::confirmdatalist[index].fcardata = new clsHipassCarData(cardata);
            }
            else
            {//rear
                strcpy((char*)commonvalues::confirmdatalist[index].rearNum,vehicleNumber.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].rearFileName1,fileName1.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].rearFileName2,fileName2.toLocal8Bit().constData());
                commonvalues::confirmdatalist[index].rFTPDatalen = ftpdata.size();
                memcpy(commonvalues::confirmdatalist[index].rFTPData,(unsigned char*)ftpdata.constData(),sizeof(unsigned char) * ftpdata.size() );
                commonvalues::confirmdatalist[index].rcardata = new clsHipassCarData(cardata);
            }
            return;
        }
    }

    //data 생성
    for(int index=0 ; index < MAX_CONFIRM_VEHICLE; index++)
    {
        if( commonvalues::confirmdatalist[index].seq == 0 && commonvalues::confirmdatalist[index].imgNumber == 0 )
        {
            commonvalues::confirmdatalist[index].seq = seq;
            commonvalues::confirmdatalist[index].imgNumber = imgNum;

            QString logstr = QString("InsertFTPData : seq-%1,imgNum-%2,front-%3,%4,%5,%6").arg(seq).arg(imgNum).arg(front_rear)
                    .arg(vehicleNumber).arg(fileName1).arg(fileName2);
            plogdlg0->logappend(LogDlg::logccu,logstr); qDebug() << logstr;

            if(front_rear == 0)
            {//front
                strcpy((char*)commonvalues::confirmdatalist[index].frontNum,vehicleNumber.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].frontFileName1,fileName1.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].frontFileName2,fileName2.toLocal8Bit().constData());
                commonvalues::confirmdatalist[index].fFTPDatalen = ftpdata.size();
                memcpy(commonvalues::confirmdatalist[index].fFTPData,(unsigned char*)ftpdata.constData(),sizeof(unsigned char) * ftpdata.size() );
                commonvalues::confirmdatalist[index].fcardata = new clsHipassCarData(cardata);
            }
            else
            {//rear
                strcpy((char*)commonvalues::confirmdatalist[index].rearNum,vehicleNumber.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].rearFileName1,fileName1.toLocal8Bit().constData());
                strcpy((char*)commonvalues::confirmdatalist[index].rearFileName2,fileName2.toLocal8Bit().constData());
                commonvalues::confirmdatalist[index].rFTPDatalen = ftpdata.size();
                memcpy(commonvalues::confirmdatalist[index].rFTPData,(unsigned char*)ftpdata.constData(),sizeof(unsigned char) * ftpdata.size() );
                commonvalues::confirmdatalist[index].rcardata = new clsHipassCarData(cardata);
            }
            return;
        }
    }


}

void mainthread::VehicleNotification_NEW()
{
    int sendindex = -1;
    quint8 seq = 0;
    quint16 imgNum = 0;
    bool bconfirm = false;
    int count = 0;

    for(int index=0 ; index < MAX_CONFIRM_VEHICLE; index++)
    {
        if( commonvalues::confirmdatalist[index].seq != 0 && commonvalues::confirmdatalist[index].imgNumber != 0 )
        {
            count++;

            if( seq == 0 ) //first index
            {
                seq = commonvalues::confirmdatalist[index].seq;
                imgNum = commonvalues::confirmdatalist[index].imgNumber;
                sendindex = index;
            }
            //순차증가
            else if( seq >= commonvalues::confirmdatalist[index].seq
                     && ( (seq - commonvalues::confirmdatalist[index].seq) < 30 ) )
            {

                seq = commonvalues::confirmdatalist[index].seq;
                imgNum = commonvalues::confirmdatalist[index].imgNumber;
                sendindex = index;
            }
            //순환  0xFF ~ 0x01
            else if( commonvalues::confirmdatalist[index].seq >= seq
                     && ( ( commonvalues::confirmdatalist[index].seq - seq) > 30 ) )
            {
                seq = commonvalues::confirmdatalist[index].seq;
                imgNum = commonvalues::confirmdatalist[index].imgNumber;
                sendindex = index;
            }

            if( commonvalues::confirmdatalist[sendindex].fFTPDatalen > 0
                    && commonvalues::confirmdatalist[sendindex].rFTPDatalen > 0)
            {
                bconfirm = true;
            }

        }

    }

    if( sendindex >= 0 )
    {
        int bsend = -1;

        if( commonvalues::confirmdatalist[sendindex].fFTPDatalen > 0
                && commonvalues::confirmdatalist[sendindex].rFTPDatalen > 0)
        {
            bsend = 0;
        }
        //앞의 조건문에서 전송 플래그는 true가 되나 만약 전/후면 영상 중에서 하나가 없는 경우에 뒤 차량의 영상이 수집되면 전송 처리가 됨.
        else if( bconfirm || count > 1)
        {
            bsend = 1;
        }

        //전후면이 다 있는 경우
        if(bsend >= 0)
        {
            QString fnumber = "xxxxxx";
            QString rnumber = "xxxxxx";
            QString confirmNumber = "xxxxxx";
            quint8 confirmValue = 0;

            if(bsend == 0)
            {

                if(commonvalues::confirmdatalist[sendindex].fFTPDatalen > 0)
                        fnumber = QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].frontNum);
                if(commonvalues::confirmdatalist[sendindex].rFTPDatalen > 0)
                        rnumber = QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].rearNum);
                if(!fnumber.contains("x"))
                {
                    confirmNumber = fnumber;
                    confirmValue = 0x01;
                }
                else if(!rnumber.contains("x"))
                {
                    confirmNumber = rnumber;
                    confirmValue = 0x02;
                }
                else
                { // all xxxxx
                    confirmNumber = fnumber;
                    confirmValue = 0x00;
                }
            }
            //전면 또는 후면만 있는 경우
            else if( bsend == 1)
            {
                if(commonvalues::confirmdatalist[sendindex].fFTPDatalen > 0)
                {
                        fnumber = QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].frontNum);
                        confirmValue = 0x01;
                        confirmNumber = fnumber;
                }
                else if(commonvalues::confirmdatalist[sendindex].rFTPDatalen > 0)
                {
                        rnumber = QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].rearNum);
                        confirmValue = 0x02;
                        confirmNumber = rnumber;
                }
            }
            else
            {
                QString logstr = QString("-------VehicleNotification_NEW - bsend Error(%1)-----------").arg(bsend);
                log->write(logstr,LOG_NOTICE);
                qDebug() << logstr;
                return;
            }

            quint8 rseq;
            if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
            {
                rseq = commonvalues::confirmdatalist[sendindex].seq;
            }
            else //commonvalues::One_Vehicle_Two_Seq
            {
                if( commonvalues::confirmdatalist[sendindex].seq == 0xFF ) rseq = 0x01;
                else rseq = commonvalues::confirmdatalist[sendindex].seq + 1;
            }

            if( commonvalues::ccuprotocol == commonvalues::CCUProtocol_New)
            {
                pccucomm0->Vehicle_Notification_NEW(rseq,
                              commonvalues::confirmdatalist[sendindex].imgNumber, fnumber, rnumber,confirmNumber,
                             confirmValue);

//                QString logstr = QString("차량번호통보전송_NEW(ch0) : seq-%1, index-%2, %3,num-(f)%4/(r)%5/(c)%6")
//                        .arg(rseq).arg(commonvalues::confirmdatalist[sendindex].imgNumber).arg(confirmValue)
//                        .arg(fnumber).arg(rnumber).arg(confirmNumber);
//                plogdlg0->logappend(LogDlg::logccu,logstr);
//                log->write(logstr,LOG_NOTICE); qDebug() << logstr;
            }
            else if(commonvalues::ccuprotocol == commonvalues::CCUProtocol_Normal )
            {
                pccucomm0->Vehicle_Notification(rseq,
                              commonvalues::confirmdatalist[sendindex].imgNumber, confirmNumber);

//                QString logstr = QString("차량번호통보전송(ch0) : seq-%1, index-%2, %3,num-(f)%4/(r)%5/(c)%6")
//                        .arg(rseq).arg(commonvalues::confirmdatalist[sendindex].imgNumber).arg(confirmValue)
//                        .arg(fnumber).arg(rnumber).arg(confirmNumber);
//                plogdlg0->logappend(LogDlg::logccu,logstr);
//                log->write(logstr,LOG_NOTICE); qDebug() << logstr;
            }

            QString FtpFilename1 = confirmValue == 0x02 ?
                  QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].rearFileName1)
                      : QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].frontFileName1);

            QString FtpFilename2 = confirmValue == 0x02 ?
                  QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].rearFileName2)
                      : QString::fromLocal8Bit((const char*)commonvalues::confirmdatalist[sendindex].frontFileName2);
            FtpFilename2 = FtpFilename2.replace(".jpg",".txt");


            QByteArray ftpdata = confirmValue == 0x02 ?
                  QByteArray((const char *)commonvalues::confirmdatalist[sendindex].rFTPData,commonvalues::confirmdatalist[sendindex].rFTPDatalen)
                    : QByteArray((const char *)commonvalues::confirmdatalist[sendindex].fFTPData,commonvalues::confirmdatalist[sendindex].fFTPDatalen);
            qDebug() << QString("confirm ftpdatalen :%1").arg(ftpdata.size());

            clsHipassCarData *cardata = confirmValue == 0x02 ?
                commonvalues::confirmdatalist[sendindex].rcardata : commonvalues::confirmdatalist[sendindex].fcardata;

            int client_count = commonvalues::clientlist.size();
            for(int index=0; index < client_count; index++)
            { //확정영상
                //X인덱스, 파일명만 필요함.
                if(commonvalues::clientlist[index]->fileNameSelect != CenterInfo::H_Char)
                {
                    commonvalues::clientlist[index]->SendVehicleInfo(FtpFilename2,QByteArray(10,(char)0x00));
                }
                else //H인덱스
                {
                    commonvalues::clientlist[index]->SendVehicleInfo(FtpFilename1,ftpdata);
                }

                //원격서버에 처리 차량번호 데이터전송 //차량통보 시에 사용함 //1채널 시 전송
               if(commonvalues::clientlist[index]->protocol_type == CenterInfo::Remote)  //H~~~.jpg
               {
                   if(confirmValue == 0x00 )
                   {
                       cardata->PROCESS_RESULT = "미확정";
                   }

                   if(commonvalues::clientlist[index]->fileNameSelect != CenterInfo::H_Char)
                   {
                       cardata->IMAGE_FILE_NAME = QString(FtpFilename2.data(),FtpFilename1.size());
                   }
                   else //H인덱스
                   {
                       cardata->IMAGE_FILE_NAME = QString(FtpFilename1.data(),FtpFilename1.size());
                   }

                   commonvalues::clientlist[index]->AddCarNoData(cardata);
               }
            }

            commonvalues::confirmdatalist[sendindex].init();
        }        
    }

}
//문자매칭여부리턴함.
QByteArray mainthread::OBUMatch(int channel, int vioresultIndex, ViolationInfo violationinf)
{
    QString logstr;
    CCUComm *pccucomm;
    pccucomm = channel == 0 ? pccucomm0 : pccucomm1;

    QString vehicleNumber = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum);
    QByteArray bmatch = QByteArray(vehicleNumber.size(),(char)commonvalues::OBU_NoMatch);

    QString obuNum = QString((const char *)violationinf.obuNumber);
    QByteArray strobuvehicle =  pdatabase->searchOBUdata(obuNum.replace("-","").toUtf8());
    if(strobuvehicle.isNull() || strobuvehicle.isEmpty())
    {
        logstr = QString("No OBU Matcing(ch:%1) : %2").arg(channel).arg(QString((const char *)violationinf.obuNumber));
        log->write(logstr,LOG_INFO);
        return bmatch;
    }

    //QByteArray obuvehicle = QByteArray::fromHex(strobuvehicle);
    QString obuvehicleNumber = pccucomm->GetVehicleNumber_byCode(strobuvehicle);
    logstr = QString("OBU Data : %1,%2,%3").arg(QString((const char *)violationinf.obuNumber)).arg(QString(strobuvehicle.toHex())).arg(obuvehicleNumber);
    log->write(logstr,LOG_INFO);

    //미인식
    if( vehicleNumber.contains("xxxxxx")) // 미인식으로 처리함.
    {
        strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,obuvehicleNumber.toLocal8Bit().constData());
        QByteArray bmismatch = QByteArray(obuvehicleNumber.size(),(char)commonvalues::OBU_MisMatch);
        return bmismatch;
    }
    //부분인식
    else if(vehicleNumber.contains("x") )
    {
        //지역/영자 인식에서오인식 가능함.
        if(vehicleNumber.size() != obuvehicleNumber.size())
        {            
            return bmatch;
        }

        int count = vehicleNumber.count("x");
        for(int index = 0; index < count; index++)
        {
            int cindex = vehicleNumber.indexOf("x");
            if(cindex >= obuvehicleNumber.count())
                 break;
            QChar cnum = obuvehicleNumber.at(cindex);
            vehicleNumber.replace(cindex,1,cnum);
            bmatch[cindex] = commonvalues::OBU_MisMatch;
        }
        strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,vehicleNumber.toLocal8Bit().constData());
    }
    //정인식
    else
    {
        if( commonvalues::obuMode == commonvalues::CCUOBU_NoReocg)
        {
            return bmatch;
        }

        //지역/영자 인식에서오인식 가능함.
        if(vehicleNumber.size() != obuvehicleNumber.size())
        {
            //자릿수가 다를 경우 CCUOBU_Mismatch이면 OBU VehicleNumber를 사용함.
            if( commonvalues::obuMode == commonvalues::CCUOBU_Mismatch)
            {
                strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,obuvehicleNumber.toLocal8Bit().constData());
                bmatch = QByteArray(obuvehicleNumber.size(),(char)commonvalues::OBU_MisMatch);
            }
            return bmatch;
        }

        QByteArray bmatchcheck = QByteArray(vehicleNumber.size(),(char)commonvalues::OBU_NoMatch);

        int count = vehicleNumber.size();
        for(int index = 0; index < count; index++)
        {
            if( vehicleNumber.at(index) != obuvehicleNumber.at(index) )
            {
                bmatchcheck[index] = commonvalues::OBU_MisMatch;
            }
            else bmatchcheck[index] = commonvalues::OBU_Match;
        }

        int mismatch_count = bmatchcheck.count(commonvalues::OBU_MisMatch);

        if(mismatch_count == 0 )
        { //동일번호판번호
        }
        else if( commonvalues::obuMode == commonvalues::CCUOBU_OneMismatch)
        {
            if(mismatch_count <= 1)
            {
                int count = bmatchcheck.size();
                for(int index=0; index < count; index++)
                {
                    if(bmatchcheck[index] == commonvalues::OBU_MisMatch)
                    {
                        vehicleNumber[index] = obuvehicleNumber[index];
                        bmatch[index] = commonvalues::OBU_MisMatch;
                    }
                }
                strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,vehicleNumber.toLocal8Bit().constData());
            }

        }
        else if( commonvalues::obuMode == commonvalues::CCUOBU_TwoMismatch)
        {
            if(mismatch_count <= 2)
            {
                int count = bmatchcheck.size();
                for(int index=0; index < count; index++)
                {
                    if(bmatchcheck[index] == commonvalues::OBU_MisMatch)
                    {
                        vehicleNumber[index] = obuvehicleNumber[index];
                        bmatch[index] = commonvalues::OBU_MisMatch;
                    }
                }
                strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,vehicleNumber.toLocal8Bit().constData());
            }

        }
        else if( commonvalues::obuMode == commonvalues::CCUOBU_Mismatch)
        {
            int count = bmatchcheck.size();
            for(int index=0; index < count; index++)
            {
                if(bmatchcheck[index] == commonvalues::OBU_MisMatch)
                {
                    vehicleNumber[index] = obuvehicleNumber[index];
                    bmatch[index] = commonvalues::OBU_MisMatch;
                }
            }
            strcpy((char*)commonvalues::vehicledatalist[channel][vioresultIndex].recognum,vehicleNumber.toLocal8Bit().constData());
        }
    }

    return bmatch;
}
