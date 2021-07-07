#include "ccucomm.h"
#include <QFileInfo>
#include "commonvalues.h"


CCUComm::CCUComm(int channel, int loglevel,QThread *parent) : QThread(parent)
{
    ThreadStop=false;
    pdata_endindex = 0;
    m_channel = channel;
    log = new Syslogger(this,"CCUComm",true,loglevel);
    m_loglevel = loglevel;
    m_sequenceindex = 1;

    m_bsyncREQsend = false;

    m_lastoutputtime = QDateTime::currentDateTime();
    m_viodelaytime = QDateTime::currentDateTime();

    m_state = CLOSE;
    m_breconnect = false;
    m_serialport = "";
    m_baudrate = 9600;
    m_databits = QSerialPort::Data8;
    m_stopbits = QSerialPort::OneStop;
    m_parity = QSerialPort::NoParity;
    m_flowcontrol = QSerialPort::NoFlowControl;
    serial = -1;

    m_bccerrorcount = 0;

    //connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
   // connect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));

    //start();
}

CCUComm::~CCUComm()
{
    //disconnect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    //disconnect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void CCUComm::stop()
{
    ThreadStop = true;
}

void CCUComm::run()
{
    int datacheckcount=0;

    //thread
    ThreadStop=false;
    m_loopcount=0;
    qint64 count = 0;
    //bool bread = false;
    //QByteArray rdata;
    char rdata[CCU_MAX_RECVBUFF];
    //qint64 rlen = 0;

    QString logstr = QString("CCU Thread(ch%1) Start").arg(m_channel);
    qDebug() << logstr;
    log->write(logstr,LOG_ERR);

    while (!ThreadStop)
    {
        try
        {
            //count = serial.bytesAvailable();
            if(serial >= 0)
                count = read(serial,rdata,sizeof(rdata));
            else
                count = 0;

            //check Input Data
            if(count > 0)
            {
                //rlen = serial.readData(rdata,CCU_MAX_RECVBUFF);
                //QByteArray rdata = serial.readAll();
                //rlen = serial.read(rdata,CCU_MAX_RECVBUFF);
                //QByteArray data;
                //data.append(serial.readAll());
                //if( rlen > 0 )
                //{
                    if( m_loglevel >= LOG_DEBUG )
                    {
                        logstr =  QString("RX_ORG(CCU/ch%1) : %2").arg(m_channel).arg(QString(QByteArray(rdata,count).toHex()));
                        log->write(logstr,LOG_DEBUG);   qDebug() << logstr;
                    }

                    Parsing(rdata,count);
                //}
                datacheckcount=0;                
            }
            else
            {
                //msleep(1);
                datacheckcount++;
                if(datacheckcount > 400) // no input data 2sec
                {
                    pdata_endindex = 0;
                    datacheckcount = 0;
                }

                if(m_bsyncREQsend)
                {

                    quint8 seq = 0x00;
                    quint8 becs = 0x01;
                    quint16 imagenum = 0xFFFF;
                    StatusREP_Send(seq, becs, imagenum);

                    m_bsyncREQsend = false;
                }
            }
            msleep(5);
            //write
            qint64 viodelay = m_viodelaytime.msecsTo(QDateTime::currentDateTime());
            qint64 sendsleep = m_lastoutputtime.msecsTo(QDateTime::currentDateTime());
            if( sendsleep >= commonvalues::ccuSendsleep)
            {
                if( VioREQData.count() > 0 && viodelay >= commonvalues::ccuViodelay)
                {
                    CCUSendData viodata = VioREQData.takeFirst();
                    writeData(viodata.senddata);
                    logstr = QString("위반확인요구(ch%1):seq-%2,imgNum-%3").arg(m_channel).arg(viodata.seq).arg(viodata.imgnumber);
                    plogdlg->logappend(LogDlg::logccu,logstr);
                    log->write(logstr,LOG_NOTICE);   qDebug() << logstr;

                }
                else if( OutputData.count() > 0)
                {
                    CCUSendData outdata = OutputData.takeFirst();
                    writeData(outdata.senddata);

                    if( outdata.command == CMD_ACK )
                    {
                        logstr = QString("ACK Send(CCU)(ch%1): seq-%2").arg(m_channel).arg(outdata.seq);
                        log->write(logstr,LOG_INFO); qDebug() << logstr;
                    }
                    else if( outdata.command == CMD_NAK )
                    {
                        QString logstr = QString("NAK Send(CCU)(ch%1): seq-%2").arg(m_channel).arg(outdata.seq);
                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                    }
                    else if( outdata.command == CMD_StatusREP )
                    {
                        logstr = QString("StatusREP Send(%1/%2/%3)").arg(outdata.seq).arg(outdata.ecs,2,16,QChar('0')).arg(outdata.imgnumber);
                        plogdlg->logappend(LogDlg::logccu,logstr);
                        log->write(logstr,LOG_INFO);
                        qDebug() << logstr;
                    }
                    else if( outdata.command == CMD_Vehicle_Notification )
                    {
                        QString vehicleNum = QString((const char *)outdata.VehicleNumber);

                        logstr = QString("차량번호통보전송(ch%1) : seq-%2, index-%3, num-%4")
                                .arg(m_channel).arg(outdata.seq).arg(outdata.imgnumber).arg(vehicleNum);
                        plogdlg->logappend(LogDlg::logccu,logstr);
                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                    }
                    else if( outdata.command == CMD_Vehicle_Notification_NEW )
                    {
                        QString fvehicleNum = QString((const char *)outdata.frontNumber);
                        QString rvehicleNum = QString((const char *)outdata.rearNumber);
                        QString cvehicleNum = QString((const char *)outdata.confirmNumber);

                        logstr = QString("차량번호통보전송_NEW(ch%1) : seq-%2, index-%3, %4,num-(f)%5/(r)%6/(c)%7")
                                .arg(m_channel).arg(outdata.seq).arg(outdata.imgnumber).arg(outdata.confirmValue)
                                .arg(fvehicleNum).arg(rvehicleNum).arg(cvehicleNum);
                        plogdlg->logappend(LogDlg::logccu,logstr);
                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                    }
                    else
                    {
                        logstr = QString("Unkown Cmd(ch%1) : %2").arg(m_channel).arg(outdata.command);
                        log->write(logstr,LOG_NOTICE); qDebug() << logstr;
                    }

                }
            }
        }
        catch( ... )
        {
            logstr = QString("--------- Error CCU Thread(ch%1) -----------").arg(m_channel);
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }
        msleep(5);
        m_loopcount++;

    }

    CloseSerial();

    logstr = QString("CCU Thread(ch%1) Stop").arg(m_channel);
    log->write(logstr,LOG_ERR);
    qDebug() << logstr;
}

int CCUComm::OpenSerial()
{
    return OpenSerial(m_serialport,m_baudrate,m_databits,m_stopbits,m_parity,m_flowcontrol);
}

int CCUComm::OpenSerial(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
{
    m_breconnect = bReconnect;
    m_serialport = portName;
    m_baudrate = baudrate;
    m_databits = databits;
    m_stopbits = stopbits;
    m_parity = parity;
    m_flowcontrol = flowcontrol;

    return OpenSerial(m_serialport,m_baudrate,m_databits,m_stopbits,m_parity,m_flowcontrol);

}

int CCUComm::OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
{
    int iret = -1;
    try{
        if( serial >= 0)
        {
            qDebug() << "CCU is already Open";
            return iret;
        }


        QString lockfile = QString("/var/lock/LCK..%1").arg(portName);
        QFileInfo fi(lockfile);
        //sdw 2016/11/11
        if( fi.exists() )
        {
//            qDebug() << QString("CCU port is in use elsewhere.");
            QString strcmd = QString("rm -rf %1").arg(lockfile);
            system(strcmd.toStdString().c_str());
//            return iret;
        }

        QString devicename = QString("/dev/%1").arg(portName);
        serial = open(devicename.toUtf8().data(), O_RDWR | O_NOCTTY | O_NONBLOCK );
        if( serial >= 0)
        {
            tcgetattr(serial,&oldtio); // save current port settings

            int speed = B115200;
            if(baudrate == 9600)
            {
                speed = B9600;
            }
            else if(baudrate == 19200)
            {
                speed = B19200;
            }
            else if(baudrate == 38400)
            {
                speed = B38400;
            }
            else if(baudrate == 115200)//115200
            {
                speed = B115200;
            }

            tcgetattr(serial,&oldtio);  // save current port settings
            memset(&newtio, 0 , sizeof(newtio));
            newtio.c_cflag = speed | CS8 | CLOCAL | CREAD;
            newtio.c_iflag = IGNPAR;
            newtio.c_oflag = 0;
            newtio.c_lflag = 0;
            newtio.c_cc[VMIN]=0;
            newtio.c_cc[VTIME]=100; //10ms
            tcflush(serial, TCIFLUSH);
            tcsetattr(serial,TCSANOW,&newtio);
            m_state = OPEN;

        }
        else {

            qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]" << QString("Open Failed %1").arg(devicename) ;
            //perror(devicename.data());
    //        if(plogdlg != NULL)
    //            plogdlg->logappend(logdlg::logspc,"[OpenSPC]" + serial.errorString());
            m_state |= OPENFAIL;
            //sdw 160912
            //open error -> serial.close;
            CloseSerial();
        }
        m_bccerrorcount = 0;
    }
    catch( ... )
    {
        QString logstr = QString("OpenSerial Exception");
        log->write(QString("Error(ch:%1)[%2] : %3")
                   .arg(m_channel).arg(Q_FUNC_INFO).arg(logstr),LOG_ERR);
        qDebug() << logstr;

        iret = -1;
    }

    return iret;
}

//int CCUComm::OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
//{
//    int iret = -1;
//    try{
//        if( serial.isOpen())
//        {
//            qDebug() << "CCU is already Open";
//            return iret;
//        }


//        QString lockfile = QString("/var/lock/LCK..%1").arg(m_serialport);
//        QFileInfo fi(lockfile);
//        //sdw 2016/11/11
//        if( fi.exists() )
//        {
////            qDebug() << QString("CCU port is in use elsewhere.");
//            QString strcmd = QString("rm -rf %1").arg(lockfile);
//            system(strcmd.toStdString().c_str());
////            return iret;
//        }

//        QString devicename = QString("/dev/%1").arg(portName);
//        serial.setPortName(devicename);
//        if (serial.open(QIODevice::ReadWrite)) {
//            if (serial.setBaudRate(baudrate) && serial.setDataBits(databits)
//                        && serial.setParity(parity) && serial.setStopBits(stopbits)
//                        && serial.setFlowControl(flowcontrol))
//            {
//                qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]CCU Port Open";
//    //            if(plogdlg != NULL)
//    //                plogdlg->logappend(logdlg::logspc,"[OpenSPC]SPC Port Open");
//                m_state = OPEN;
//                iret = 0;
//                emit sigReceiveHandler(m_channel,CMD_Connected,"",NULL);

//            }
//            else {
//                 qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]" + serial.errorString();
//    //             if(plogdlg != NULL)
//    //                 plogdlg->logappend(logdlg::logspc,"[OpenSPC]" +serial.errorString());
//                 m_state |= OPENFAIL;
//                 //sdw 160912
//                 //open error -> serial.close;
//                 CloseSerial();
//            }
//        }
//        else {

//            qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]" + serial.errorString();
//    //        if(plogdlg != NULL)
//    //            plogdlg->logappend(logdlg::logspc,"[OpenSPC]" + serial.errorString());
//            m_state |= OPENFAIL;
//            //sdw 160912
//            //open error -> serial.close;
//            CloseSerial();
//        }
//        m_bccerrorcount = 0;
//    }
//    catch( ... )
//    {
//        QString logstr = QString("OpenSerial Exception");
//        log->write(QString("Error(ch:%1)[%2] : %3")
//                   .arg(m_channel).arg(Q_FUNC_INFO).arg(logstr),LOG_ERR);
//        qDebug() << logstr;

//        iret = -1;
//    }

//    return iret;
//}

//void CCUComm::CloseSerial()
//{

//    serial.close();
//    QString logstr = "[CloseSerial]CCU Port Close";
//    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
//    //if(plogdlg != NULL) plogdlg->logappend(logdlg::logspc,logstr);
//    emit sigReceiveHandler(m_channel,CMD_Disconnected,"",NULL);

//    m_state |= CLOSE;
//}

void CCUComm::CloseSerial()
{
    if(serial >= 0)
    {
        tcsetattr(serial,TCSANOW,&oldtio);
        close(serial);
    }
    QString logstr = "[CloseSerial]CCU Port Close";
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    //if(plogdlg != NULL) plogdlg->logappend(logdlg::logspc,logstr);
    emit sigReceiveHandler(m_channel,CMD_Disconnected,"",NULL);

    m_state |= CLOSE;
}

void CCUComm::Parsing(char *rdata, int rdatalen)
{
    QString logstr;
    int data_count = rdatalen;
    int datalenchek = pdata_endindex + data_count;
    if(datalenchek >= CCU_MAX_RECVBUFF)
    {
        data_count = CCU_MAX_RECVBUFF - pdata_endindex - 1;

        logstr = QString("Receive Data overflow(CCU ch%1) : %2 ( %3 -> %4 )")
                .arg(m_channel).arg(datalenchek).arg(rdatalen).arg(data_count);
        log->write(logstr,LOG_ERR);   qDebug() << logstr;

    }

    memcpy(&pdata[pdata_endindex],(quint8 *)rdata,sizeof(unsigned char)*data_count);
    pdata_endindex += data_count;


    int stxindex = 0;
    while(stxindex < pdata_endindex)
    {
        int pdatalen = pdata_endindex - stxindex;
        if( pdatalen < 8)
        { //Save rest data
            pdatalen = pdata_endindex - stxindex;
            quint8 data_p[pdatalen];
            memcpy(data_p,&pdata[stxindex],sizeof(quint8) * pdatalen);
            memcpy(pdata, data_p,sizeof(quint8) * pdatalen);
            pdata_endindex = pdatalen;
            return;
        }

        if( pdata[stxindex] == DLE && pdata[stxindex + 1] == STX )
        {
            int etxindex = stxindex + 2;
            uint datalen = pdata_endindex - 1;
            while(etxindex < datalen)
            {
                if( pdata[etxindex] == DLE && pdata[etxindex - 1] == DLE )
                    etxindex += 2;
                else if( pdata[etxindex] == ETX && pdata[etxindex - 1] == DLE )
                {
                    //process cmd
                    int cmdlen = etxindex - stxindex + 2; //bcc 1byte [+]
                    quint8 cmddata[cmdlen];
                    memcpy(cmddata,&pdata[stxindex],sizeof(quint8)*cmdlen);
                    ProcessCommand(cmddata,cmdlen);
                    stxindex = etxindex + 2;
                    break;
                }
                else
                    etxindex++;
            }
            if( etxindex >= datalen)
            { //Save rest data
                int rest_count;
                rest_count = pdata_endindex - stxindex;
                quint8 data_p[rest_count];
                memcpy(data_p,&pdata[stxindex],sizeof(quint8) * rest_count);
                memcpy(pdata, data_p,sizeof(quint8) * rest_count);
                pdata_endindex = rest_count;
                return;
            }
        }
        else
            stxindex++;
    }
    if(stxindex >= pdata_endindex)
    { // no data
        pdata_endindex=0;
    }
}

void CCUComm::ProcessCommand(quint8 *cmddata, int cmdlen)
{
    //bigendian
    QString logstr;
    try
    {
        QByteArray qcommandData = QByteArray((const char*)cmddata,cmdlen);
        QByteArray Command = Minus0x10(qcommandData);
        if(Command.isNull() || Command.isEmpty())
        {
            qDebug() << QString("ProcessCommand-Minus0x10 is zero");
            return;
        }
        logstr = QString("RX(CCU/ch%1) : %2").arg(m_channel).arg(QString(Command.toHex()));
        qDebug() << logstr;
        log->write(logstr,LOG_DEBUG);

        quint8 Opcode = Command[3];

        //Check BCC
        if (CheckBCC(Command))
        {//ACK
            if( Opcode != CMD_ACK && Opcode != CMD_NAK)
            {
                    ACK_Send(Command[4]);//전송연번
                    m_bccerrorcount = 0;
            }
        }
        else
        {//NAK
            m_bccerrorcount++;
            if( Opcode != CMD_ACK && Opcode != CMD_NAK)
                    NAK_Send(Command[4]);
            return;
        }

        QString strCommand = "";

        if( Opcode == CMD_ACK)
        {
            logstr = QString("Rcv ACK(CCU)(ch%1) : seq - %2").arg(m_channel).arg(QString::number((quint8)Command[4]));
            log->write(logstr,LOG_INFO);
            qDebug() << logstr;

            strCommand = QString::number(Command[4]);  //전송연번
        }
        else if( Opcode == CMD_NAK)
        {
            logstr = QString("Rcv NAK(CCU)(ch%1) : seq - %2").arg(m_channel).arg(QString::number((quint8)Command[4]));
            log->write(logstr,LOG_INFO);
            plogdlg->logappend(LogDlg::logccu,logstr);
            qDebug() << logstr;

            strCommand = QString::number(Command[4]);  //전송연번
        }
        else if( Opcode == CMD_StatusREQ)
        {
            QString seq = QString::number((quint8)Command[4]);
            QString sDateNow = QString("%1%2%3%4%5%6")
                    .arg(ArrtoUint(Command.mid(5,2)),4,10,QChar('0')).arg((quint8)Command[7],2,10,QChar('0')).arg((quint8)Command[8],2,10,QChar('0'))
                    .arg((quint8)Command[9],2,10,QChar('0')).arg((quint8)Command[10],2,10,QChar('0')).arg((quint8)Command[11],2,10,QChar('0'));   //yyyyMMddHHmmss
            strCommand = QString("%1,%2").arg(seq).arg(sDateNow);
            emit sigReceiveHandler(m_channel,CMD_StatusREQ,strCommand,NULL);
            QString logstr = "상태요청: " + strCommand;
            log->write(logstr,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,logstr);
            qDebug() << logstr;

            //endian change -> #include <QtEndian> ref
        }
        else if( Opcode == CMD_WORKSTART)
        {
            QString seq = QString::number(Command[4]);
            QString sWorkStartTime = QString("%1%2%3%4%5%6")
                    .arg(ArrtoUint(Command.mid(5,2)),4,10,QChar('0')).arg((quint8)Command[7],2,10,QChar('0')).arg((quint8)Command[8],2,10,QChar('0'))
                    .arg((quint8)Command[9],2,10,QChar('0')).arg((quint8)Command[10],2,10,QChar('0')).arg((quint8)Command[11],2,10,QChar('0')); //근무개시시간 //yyyyMMddHHmmss
            QString sWorkType = QString::number(Command[12]);  //근무형태 0 : 정상근무 , 1 : 유지보수
            quint8 WorkNumber1 = Command[13]; //근무번호 - BCD(01~99), 차로
            quint8 WorkNumber2 = Command[14]; //근무번호 - BCD, 근무인련번호
            QString sWorkDate = QString("%1%2%3")
                    .arg(ArrtoUint(Command.mid(15,2)),4,10,QChar('0')).arg((quint8)Command[17],2,10,QChar('0')).arg((quint8)Command[18],2,10,QChar('0'));  //근무일자 //yyyyMMdd

            commonvalues::Remotedata->WORK_START_DATETIME = sWorkStartTime + "000"; //msec추가
            commonvalues::Remotedata->LANE_NO = QString("%1").arg(WorkNumber1,2,16,QChar('0'));

            emit sigReceiveHandler(m_channel,CMD_WORKSTART,sWorkStartTime,NULL);

            strCommand = QString("근무개시통보 : 전송연번-%1,개시시간-%2,근무형태-%3,근무번호-%4/%5,근무일자-%6")
                    .arg(seq).arg(sWorkStartTime).arg(sWorkType).arg(WorkNumber1,2,16).arg(WorkNumber2,2,16).arg(sWorkDate);
            log->write(strCommand,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,strCommand);
            qDebug() << QString("CMD_WORKSTART : %1").arg(strCommand);

        }
        else if( Opcode == CMD_WORKEND)
        {
            QString seq = QString::number(Command[4]);
            QString sWorkEndTime = QString("%1%2%3%4%5%6")
                    .arg(Command.mid(5,2).toUInt(),4,10,QChar('0')).arg((quint8)Command[7],2,10,QChar('0')).arg((quint8)Command[8],2,10,QChar('0'))
                    .arg((quint8)Command[9],2,10,QChar('0')).arg((quint8)Command[10],2,10,QChar('0')).arg((quint8)Command[11],2,10,QChar('0')); //근무개시시간 //yyyyMMddHHmmss
            QString sWorkType = QString::number(Command[12]);  //근무형태 0 : 정상근무 , 1 : 유지보수
            quint8 WorkNumber1 = Command[13]; //근무번호 - BCD(01~99), 차로
            quint8 WorkNumber2 = Command[14]; //근무번호 - BCD, 근무인련번호
            QString sWorkDate = QString("%1%2%3")
                    .arg(Command.mid(15,2).toUInt(),4,10,QChar('0')).arg((quint8)Command[17],2,10,QChar('0')).arg((quint8)Command[18],2,10,QChar('0'));  //근무일자 //yyyyMMdd
            strCommand = QString("근무종료통보 : 전송연번-%1,개시시간종료시간2,근무형태-%3,근무번호-%4/%5,근무일자-%6")
                    .arg(seq).arg(sWorkEndTime).arg(sWorkType).arg(WorkNumber1,2,16).arg(WorkNumber2,2,16).arg(sWorkDate);
            log->write(strCommand,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,strCommand);
            qDebug() << QString("CMD_WORKEND : %1").arg(strCommand);
        }
        else if( Opcode == CMD_VIOLATION_REP)
        {
            emit sigReceiveHandler(m_channel,CMD_VIOLATION_REP,"",Command);
        }
        else if( Opcode == CMD_VIOLATION_REP_NEW)
        { //data  bigendian
            emit sigReceiveHandler(m_channel,CMD_VIOLATION_REP_NEW,"",Command);
        }
        else if( Opcode == CMD_VIOLATION_SYNC)
        {
            QString seq = QString::number(Command[4]);
            quint16 uViolationNumber = (quint16)ArrtoUint(Command.mid(5,2));
            strCommand = QString("%1,%2").arg(seq).arg(uViolationNumber);
            logstr = QString("위반번호 Sync : 전송연번-%1, 위반번호-%2").arg(seq).arg(uViolationNumber);
            emit sigReceiveHandler(m_channel,CMD_VIOLATION_SYNC,strCommand,NULL);
            log->write(logstr,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,logstr);
            qDebug() << QString("CMD_VIOLATION_SYNC : %1").arg(logstr);
        }
        else if( Opcode == CMD_SPEC_ENTER_BEGIN)
        {
            QString seq = QString::number(Command[4]);
            strCommand = QString("입구 특별발행개시 : 전송연번-%1").arg(seq);
            log->write(strCommand,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,strCommand);
            qDebug() << QString("CMD_SPEC_ENTER_BEGIN : %1").arg(strCommand);
        }
        else if( Opcode == CMD_SPEC_ENTER_END)
        {
            QString seq = QString::number(Command[4]);
            strCommand = QString("입구 특별발행종료 : 전송연번-%1").arg(seq);
            log->write(strCommand,LOG_NOTICE);
            plogdlg->logappend(LogDlg::logccu,strCommand);
            qDebug() << QString("CMD_SPEC_ENTER_END : %1").arg(strCommand);
        }
        else if( Opcode == CMD_CONFIRM_INFO)
        {
            QString seq = QString::number(Command[4]);
            quint16 ProcessManagedNum = (quint16)Command.mid(5,2).toUInt();
            QString sWorkDate = QString("%1%2%3")
                    .arg(ArrtoUint(Command.mid(7,2)),4,10).arg((quint8)Command[9],2,10).arg((quint8)Command[10],2,10);  //근무일자 //yyyyMMdd
            quint8 WorkNumber1 = (quint8)Command[11]; //근무번호 - BCD(01~99), 차로
            quint8 WorkNumber2 = (quint8)Command[12]; //근무번호 - BCD, 근무인련번호
            quint32 ProcessNumber = ArrtoUint(Command.mid(13,4));
            quint8 ConfirmLocation = (quint8)Command[17];  //front - 0x01 , rear - 0x02

            strCommand = QString("%1,%2,%3,%4,%5,%6,%7")
                    .arg(seq).arg(ProcessManagedNum).arg(sWorkDate)
                    .arg(WorkNumber1).arg(WorkNumber2).arg(ProcessNumber)
                    .arg(ConfirmLocation);
            emit sigReceiveHandler(m_channel,CMD_CONFIRM_INFO,strCommand,NULL);
            logstr = QString("확정정보 : %1").arg(strCommand);
            log->write(logstr,LOG_NOTICE);  qDebug() << logstr;
            plogdlg->logappend(LogDlg::logccu,logstr);

        }
        else
        {
            qDebug() << QString("CCU Command is Unknown");
        }
    }
    catch( ... )
    {
        QString logstr = QString("ProcessCommand Exception");
        log->write(QString("Error(ch:%1)[%2] : %3")
                   .arg(m_channel).arg(Q_FUNC_INFO).arg(logstr),LOG_ERR);
        qDebug() << logstr;
    }
}

QByteArray CCUComm::Minus0x10(QByteArray rxdata)
{
    QByteArray processdata;
    try
    {
        int i10count = 0;
        int i10checkindex = rxdata.length() - 3;
        int i10checkstart = 2;

        for( int i = i10checkstart; i < i10checkindex; i++)
        {
            if( rxdata[i] == DLE && rxdata[i + 1] == DLE)
            {
                i10count++;
                i++;
            }
        }

        processdata = rxdata.mid(0,2);
        int iIndex = i10checkstart;
        int orgindex = i10checkstart;
        for(; orgindex < i10checkindex;)
        {
            if( rxdata[orgindex] == DLE && rxdata[orgindex + 1] == DLE)
            {
                processdata.append(rxdata[orgindex]);
                orgindex++;
            }
            else
                processdata.append(rxdata[orgindex]);

            iIndex++;
            orgindex++;
        }
        processdata.append(rxdata.mid(i10checkindex));
    }
    catch ( ... )
    {
        QString logstr = QString("Minus0x10(CCU) Exception");
        log->write(QString("Error(ch:%1)[%2] : %3")
                   .arg(m_channel).arg(Q_FUNC_INFO).arg(logstr),LOG_ERR);
        qDebug() << logstr;

        processdata.clear();
    }
    return processdata;

}

void CCUComm::SerialDeviceSetting()
{
    CloseSerial();
    // input data clear
    InputData.clear();
    pdata_endindex = 0;
    //m_transcount=0;
    // spc open
    OpenSerial();

    QString logstr = QString("Device Reconnection : %1").arg(m_serialport);
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    //log->write(logstr,LOG_INFO);
}

void CCUComm::SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
{
    CloseSerial();
    // input data clear
    InputData.clear();
    pdata_endindex = 0;
    //m_transcount=0;
    // spc open
    OpenSerial(bReconnect, portName, baudrate, databits, stopbits, parity, flowcontrol);

    QString logstr = QString("Device Reconnection : %1").arg(m_serialport);
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    //log->write(logstr,LOG_INFO);

}

void CCUComm::writeData(QByteArray data)
{
    //serial close -> reconnect
    if(m_breconnect && serial < 0)
    {
        OpenSerial();
    }

    if(serial >= 0)
    {
        int irtn = write(serial,data.data(),data.length());
        if( irtn < 0)
        {
            QString logstr = QString("Serial Write Failed(%1)").arg(m_serialport);
            log->write(logstr,LOG_ERR); qDebug() << logstr;
            CloseSerial();
        }
        else
        {
//        if(m_loglevel >= LOG_DEBUG)
//        {
            QString logstr = QString("%1 writeData-len(%2): %3")
                    .arg(m_serialport).arg(data.size()).arg(QString(data.toHex()));
            qDebug() << logstr;
//            log->write(logstr,LOG_DEBUG);
//        }
        }
    }
    else
    {
        QString logstr = QString("[writeData]%1 Port Close").arg(m_serialport);
        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    }

    m_lastoutputtime = QDateTime::currentDateTime();

}
//void CCUComm::writeData(QByteArray data)
//{
//    //serial close -> reconnect
//    if(m_breconnect && !serial.isOpen())
//    {
//        OpenSerial();
//    }

//    //sdw 2016/10/12
//   // wmutex.lock();

//    //sdw 2016/09/12  Check serial open
//    if(serial.isOpen())
//    {
//        serial.write(data);
//        serial.waitForBytesWritten(1000);
////        if(m_loglevel >= LOG_DEBUG)
////        {
//            QString logstr = QString("%1 writeData-len(%2): %3")
//                    .arg(m_serialport).arg(data.size()).arg(QString(data.toHex()));
//            qDebug() << logstr;
////            log->write(logstr,LOG_DEBUG);
////        }
//    }
//    else
//    {
//        QString logstr = QString("[writeData]%1 Port Close").arg(m_serialport);
//        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
//    }

//    m_lastoutputtime = QDateTime::currentDateTime();
////    unsigned int sendsleep = commonvalues::ccuSendsleep;
////    if( sendsleep > 0)
////    {
////        if( sendsleep > 500)
////             sendsleep = 500;

////        msleep(sendsleep);
////    }
//   // wmutex.unlock();

//}

QByteArray CCUComm::MakeSendPacket(quint8 Opcode, quint8 Seq, QByteArray Data)
{
    QByteArray SendData;
    QByteArray repArr;

    try
    {
        int datalen = 0;
        if( !Data.isNull() && !Data.isEmpty() ) datalen = Data.length();

        SendData.append((quint8)DLE);
        SendData.append((quint8)STX);
        //bcc start
        SendData.append((quint8)datalen + 2); // code + seq + data
        SendData.append(Opcode);
        SendData.append(Seq);
        if( datalen > 0)
            SendData.append(Data);
        SendData.append((quint8)DLE);
        SendData.append((quint8)ETX);

        QByteArray bccdata = SendData.mid(3, 2 + datalen);
        quint8 bcc = MakeBCC(bccdata);

        SendData.append(bcc);

        int i10Count = 0;
        int senddatalen = SendData.length() - 3;
        for( int i=2; i < senddatalen; i++)
        {
            if(SendData[i] == (quint8)DLE) i10Count++;
        }

        repArr.append(SendData.mid(0,2));
        for( int i = 2; i < senddatalen; i++)
        {
            if(SendData[i] == (quint8)DLE)
                repArr.append(2,SendData[i]);
            else
                repArr.append(SendData[i]);
        }
        repArr.append(SendData.mid(senddatalen,3));

        QString logstr = QString("TX(CCU) : %1").arg(QString(repArr.toHex()));
        qDebug() << logstr;
        log->write(logstr,LOG_DEBUG);

    }
    catch( ... )
    {
        QString logstr =  QString("MakeSendPacket Exception");
        qDebug() << logstr;
        log->write(logstr,LOG_ERR);
        repArr.clear();
    }

    return repArr;
}

quint8 CCUComm::MakeBCC(QByteArray data)
{
    quint8 bcc = 0x00;
    int datalen = data.length();
    for (int i = 0; i < datalen; i++)
    {
        bcc ^= data[i];
    }
    return bcc;
}

bool CCUComm::CheckBCC(QByteArray data)
{
    bool brtn = false;

    int datalen = data.length() - 6;
    QByteArray checkdata = data.mid(3,datalen); // LEN ~ DATA

    quint8 rbcc = data[data.length() - 1];
    quint8 bcc = MakeBCC(checkdata);

    if (rbcc == bcc) brtn = true;
    else
    {
        QString logstr =  QString("BCC Error(CCU): r%1/c%2(%3)").arg(rbcc).arg(bcc).arg(QString(data.toHex()));
        log->write(logstr,LOG_ERR);  qDebug() << logstr;

    }

    return brtn;
}

QByteArray CCUComm::Uint16toArr(quint16 number)
{
    QByteArray conbytearr;
    conbytearr.append((quint8)(number >> 8));
    conbytearr.append((quint8)(number & 0x00FF));
    return conbytearr;
}

quint32 CCUComm::ArrtoUint(QByteArray bdata)
{
    //bigendian
        int count = bdata.length();
        quint32 iret = 0;

        for(int index=0; index < count; index++)
        {
            if(index != 0 ) iret <<= 8;;
            iret += (quint8)bdata[index];
        }
        return iret;
}


void CCUComm::ACK_Send(quint8 seq)
{
    QByteArray senddata = MakeSendPacket(CMD_ACK,seq,NULL);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr = QString("ACK_Send is NULL/Empty");
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }
    CCUSendData ccudata;
    ccudata.command = CMD_ACK;
    ccudata.seq = seq;
    ccudata.senddata = senddata;
    OutputData.append(ccudata);


//    writeData(senddata);
//    QString logstr = QString("ACK Send(CCU)(ch%1): seq-%2").arg(m_channel).arg(seq);
//    log->write(logstr,LOG_INFO); qDebug() << logstr;
}

void CCUComm::NAK_Send(quint8 seq)
{
    QByteArray senddata = MakeSendPacket(CMD_NAK,seq,NULL);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr = QString("NAK_Send is NULL/Empty");
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }
    CCUSendData ccudata;
    ccudata.command = CMD_NAK;
    ccudata.seq = seq;
    ccudata.senddata = senddata;
    OutputData.append(ccudata);

//    writeData(senddata);
//    QString logstr = QString("NAK Send(CCU)(ch%1): seq-%2").arg(m_channel).arg(seq);
//    log->write(logstr,LOG_NOTICE); qDebug() << logstr;

}

void CCUComm::StatusREP_Send(quint8 seq, quint8 ecs, quint16 imgnumber)
{
    QByteArray bimgnumber = Uint16toArr(imgnumber);
    QByteArray data;
    data.append(ecs);
    data.append(bimgnumber);

    QByteArray senddata = MakeSendPacket(CMD_StatusREP,seq,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr = QString("StatusREP_Send is NULL/Empty(%1)").arg(seq);
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }

    CCUSendData ccudata;
    ccudata.command = CMD_StatusREP;
    ccudata.seq = seq;
    ccudata.ecs = ecs;
    ccudata.imgnumber = imgnumber;
    ccudata.senddata = senddata;
    OutputData.append(ccudata);

//    writeData(senddata);

//    QString logstr = QString("StatusREP Send(%1/%2/%3)").arg(seq).arg(ecs,2,16,QChar('0')).arg(imgnumber);
//    plogdlg->logappend(LogDlg::logccu,logstr);
//    log->write(logstr,LOG_INFO);
//    qDebug() << logstr;

}

void CCUComm::VIOLATION_REQ_Send(quint8 seq, quint16 imgnumber)
{
    //seq++;  //위반촬영장치는 전송연번증가는 "위반확인요구"시와 "차량번호 통보"시에만 증가한다.
    QByteArray bimgnumber = Uint16toArr(imgnumber);
    QByteArray data;
    data.append((quint8)0x00); // Trig 수신정상 및 영상촬영 정상
    data.append(bimgnumber);

    QByteArray senddata = MakeSendPacket(CMD_VIOLATION_REQ,seq,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr =  QString("VIOLATION_REQ_Send is NULL/Empty(%1)").arg(seq);
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }

    m_viodelaytime = QDateTime::currentDateTime();
    CCUSendData ccudata;
    ccudata.command = CMD_VIOLATION_REQ;
    ccudata.seq = seq;
    ccudata.imgnumber = imgnumber;
    ccudata.senddata = senddata;
    VioREQData.append(ccudata);


//    unsigned int delayms = commonvalues::ccuViodelay;
//    if(delayms > 0)
//    {
//        if(delayms > 500)
//                delayms = 500;

//        msleep(delayms);
//    }

//    writeData(senddata);
//    QString logstr = QString("위반확인요구(ch%1):seq-%2,imgNum-%3").arg(m_channel).arg(seq).arg(imgnumber);
//    plogdlg->logappend(LogDlg::logccu,logstr);
//    log->write(logstr,LOG_NOTICE);   qDebug() << logstr;

}

void CCUComm::Vehicle_Notification(quint8 seq, quint16 imgnumber, QString vehiclenumber)
{
    //seq++;  //위반촬영장치는 전송연번증가는 "위반확인요구"시와 "차량번호 통보"시에만 증가한다.
    QByteArray bimgnumber = Uint16toArr(imgnumber);
    QByteArray data;
    data.append(bimgnumber);
    QByteArray bvehiclenumber = GetVehicleNumberByte(vehiclenumber);
    data.append(bvehiclenumber);
    //dummy  0x02 0x00 0x00 0x00 0x00
    data.append(QByteArray(1,0x02));
    data.append(QByteArray(4,0x00));

    QByteArray senddata = MakeSendPacket(CMD_Vehicle_Notification,seq,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr =  QString("Vehicle_Notification is NULL/Empty(%1)").arg(seq);
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }

    CCUSendData ccudata;
    ccudata.command = CMD_Vehicle_Notification;
    ccudata.seq = seq;
    ccudata.imgnumber = imgnumber;
    //ccudata.VehicleNumber = vehiclenumber;
    strcpy((char*)ccudata.VehicleNumber ,vehiclenumber.toLocal8Bit().constData());
    ccudata.senddata = senddata;
    OutputData.append(ccudata);
//    writeData(senddata);
//    qDebug() << QString("차량번호통보(ch%1):seq-%2,%3/%4").arg(m_channel).arg(seq).arg(vehiclenumber).arg(QString(bvehiclenumber.toHex()));

}

void CCUComm::Vehicle_Notification_NEW(quint8 seq, quint16 imgnumber, QString frontNumber, QString rearNumber, QString confirmNumber, quint8 confirmVaule)
{
    //seq++;  //위반촬영장치는 전송연번증가는 "위반확인요구"시와 "차량번호 통보"시에만 증가한다.
    QByteArray bimgnumber = Uint16toArr(imgnumber);
    QByteArray data;
    data.append(bimgnumber);
    QByteArray bfrontNumber = GetVehicleNumberByte(frontNumber);
    QByteArray brearNumber = GetVehicleNumberByte(rearNumber);
    QByteArray bconfirmNumber = GetVehicleNumberByte(confirmNumber);
    data.append(bfrontNumber);
    data.append(brearNumber);
    data.append(bconfirmNumber);
    data.append(confirmVaule);
    data.append(QByteArray(10,0x00));

    QByteArray senddata = MakeSendPacket(CMD_Vehicle_Notification_NEW,seq,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        QString logstr =  QString("Vehicle_Notification_NEW is NULL/Empty(%1)").arg(seq);
        qDebug() << logstr;
        log->write(logstr,LOG_NOTICE);
        return;
    }

    CCUSendData ccudata;
    ccudata.command = CMD_Vehicle_Notification_NEW;
    ccudata.seq = seq;
    ccudata.imgnumber = imgnumber;
    ccudata.confirmValue = confirmVaule;
    strcpy((char*)ccudata.frontNumber ,frontNumber.toLocal8Bit().constData());
    strcpy((char*)ccudata.rearNumber ,rearNumber.toLocal8Bit().constData());
    strcpy((char*)ccudata.confirmNumber ,confirmNumber.toLocal8Bit().constData());
    //ccudata.frontNumber = frontNumber;
    //ccudata.rearNumber = rearNumber;
    //ccudata.confirmNumber = confirmNumber;
    ccudata.senddata = senddata;
    OutputData.append(ccudata);

//    writeData(senddata);
//    qDebug() << QString("차량번호통보_NEW(ch%1):seq-%2,%3,%4/(f)%5,(r)%6,(c)%7")
//                .arg(m_channel).arg(seq).arg(confirmNumber).arg(confirmVaule)
//                .arg(QString(bfrontNumber.toHex()))
//                .arg(QString(brearNumber.toHex()))
//                .arg(QString(bconfirmNumber.toHex()));

}

QByteArray CCUComm::GetVehicleNumberByte(QString VehicleNumber)
{
    QString sVehicleNumber = VehicleNumber;
    bool bCommercial = false;
    if( VehicleNumber.contains("영"))
    {
        bCommercial = true;
        sVehicleNumber = sVehicleNumber.replace("영","");  // 영자는 번호에서 삭제
    }
    QByteArray bNumber;

    if( sVehicleNumber.contains("x")) // 부분 미인식도 미인식으로 처리함.
    {
        bNumber = QByteArray(5,0xFF);
        return bNumber;
    }

    int vlen =  sVehicleNumber.length();
    int blockcount = 1;
    int inumblock[vlen];
    bool bnum_arry[vlen];
    for( int index = 0, blockindex = 1; index < vlen; index++, blockindex++)
    {
        bnum_arry[index] = sVehicleNumber[index].isDigit();
        if( index != 0 && (bnum_arry[index - 1] != bnum_arry[index]))
        {
            blockindex = 1;
            blockcount++;
        }
        inumblock[index] = blockindex;
    }  // 123가1234  -> { 1, 2, 3, 1, 1,2,3,4 }

    //check vehiclenumber
    if( sVehicleNumber.contains("외교"))  //외교번호판,  외교XXXXXX(숫자6자리, 다른문자 제외 )
    {
        QString sNum1 = "외교";
        sVehicleNumber = sVehicleNumber.replace("외교", "");
        sVehicleNumber = sVehicleNumber.replace("-", "");
        if( sVehicleNumber.length() != 6 )
        {
            bNumber = QByteArray(5,0xFF);   //미인식 처리
            return bNumber;
        }

        QString sNum2 = sVehicleNumber.mid(0, 2);
        QString sNum3 = "외";
        QString sNum4 = sVehicleNumber.mid(2, 2);
        QString sNum5 = sVehicleNumber.mid(4, 2);

        bool ok;
        bNumber[0] = GetRegionCode(sNum1, bCommercial);
        bNumber[1] = (quint8)sNum2.toUInt(&ok,16);
        bNumber[2] = GetCarClass(sNum3);
        bNumber[3] = (quint8)sNum4.toUInt(&ok,16);
        bNumber[4] = (quint8)sNum5.toUInt(&ok,16);
    }
    else if(blockcount == 3) //통합 번호판 : 12가1234, 123가1234
    {
        if(sVehicleNumber.length() == 8) //신규3자리 번호판
        {
            QString sNum1 = sVehicleNumber.mid(0, 1);
            QString sNum2 = sVehicleNumber.mid(1, 2);
            QString sNum3 = sVehicleNumber.mid(3, 1);
            QString sNum4 = sVehicleNumber.mid(4, 2);
            QString sNum5 = sVehicleNumber.mid(6, 2);

            bool ok;
            bNumber[0] = GetRegionCode(sNum1, bCommercial);
            bNumber[1] = (quint8)sNum2.toUInt(&ok, 16);
            bNumber[2] = GetCarClass(sNum3);
            bNumber[3] = (quint8)sNum4.toUInt(&ok, 16);
            bNumber[4] = (quint8)sNum5.toUInt(&ok, 16);
        }
        else
        {
            QString sNum1 = "";
            QString sNum2 = sVehicleNumber.mid(0, 2);
            QString sNum3 = sVehicleNumber.mid(2, 1);
            QString sNum4 = sVehicleNumber.mid(3, 2);
            QString sNum5 = sVehicleNumber.mid(5, 2);

            bool ok;
            bNumber[0] = GetRegionCode(sNum1, bCommercial);
            bNumber[1] = (quint8)sNum2.toUInt(&ok, 16);
            bNumber[2] = GetCarClass(sNum3);
            bNumber[3] = (quint8)sNum4.toUInt(&ok, 16);
            bNumber[4] = (quint8)sNum5.toUInt(&ok, 16);
        }
    }
    else
    {
        QString sNum1 = sVehicleNumber.mid(0, 2);
        int len = inumblock[3];  // 7 , 07에 대한 길이를 가져옴.
        QString sNum2 = sVehicleNumber.mid(2, len);
        QString sNum3 = sVehicleNumber.mid(2+len, 1);
        QString sNum4 = sVehicleNumber.mid(3+len, 2);
        QString sNum5 = sVehicleNumber.mid(5+len, 2);

        bool ok;
        bNumber[0] = GetRegionCode(sNum1,bCommercial);
        bNumber[1] = (quint8)sNum2.toUInt(&ok, 16);
        bNumber[2] = GetCarClass(sNum3);
        bNumber[3] = (quint8)sNum4.toUInt(&ok, 16);
        bNumber[4] = (quint8)sNum5.toUInt(&ok, 16);

    }

    return bNumber;
}

QString CCUComm::GetVehicleNumber_byCode(QByteArray bNumber)
{
    QString sVehicleNumber = "";

    QString sNum1 = GetRegion_byCode((quint8)bNumber[0]);
    QString sNum2 = QString("%1").arg((quint8)bNumber[1],2,16,QChar('0'));
    QString sNum3 = GetCarClass_byCode((quint8)bNumber[2]);
    QString sNum4 = QString("%1").arg((quint8)bNumber[3],2,16,QChar('0'));
    QString sNum5 = QString("%1").arg((quint8)bNumber[4],2,16,QChar('0'));

    // 외교12외1234 -> 외교121234

    sVehicleNumber = QString("%1%2%3%4%5").arg(sNum1).arg(sNum2).arg(sNum3).arg(sNum4).arg(sNum5);
    // 영서울06다1234 -> 서울06다1234영
    if (sVehicleNumber.contains("영"))
    {
        sVehicleNumber = sVehicleNumber.replace("영", "");
        sVehicleNumber += "영";
    }
    return sVehicleNumber;
}

quint8 CCUComm::GetRegionCode(QString sNum1, bool bCommercial)  //지역, '영'자
{
    quint8 bNumber = 99;
    if (sNum1 == "서울") bNumber = 0x00;
    else if (sNum1 == "부산") bNumber = 0x01;
    else if (sNum1 == "인천") bNumber = 0x02;
    else if (sNum1 == "대구") bNumber = 0x03;
    else if (sNum1 == "광주") bNumber = 0x04;
    else if (sNum1 == "대전") bNumber = 0x05;
    else if (sNum1 == "경기") bNumber = 0x06;
    else if (sNum1 == "강원") bNumber = 0x07;
    else if (sNum1 == "충북") bNumber = 0x08;
    else if (sNum1 == "충남") bNumber = 0x09;
    else if (sNum1 == "전북") bNumber = 0x10;
    else if (sNum1 == "전남") bNumber = 0x11;
    else if (sNum1 == "경북") bNumber = 0x12;
    else if (sNum1 == "경남") bNumber = 0x13;
    else if (sNum1 == "제주") bNumber = 0x14;
    else if (sNum1 == "울산") bNumber = 0x15;
    else if (sNum1 == "세종") bNumber = 0x16;

    else if (sNum1 == "0") bNumber = 0x80;
    else if (sNum1 == "1") bNumber = 0x81;
    else if (sNum1 == "2") bNumber = 0x82;
    else if (sNum1 == "3") bNumber = 0x83;
    else if (sNum1 == "4") bNumber = 0x84;
    else if (sNum1 == "5") bNumber = 0x85;
    else if (sNum1 == "6") bNumber = 0x86;
    else if (sNum1 == "7") bNumber = 0x87;
    else if (sNum1 == "8") bNumber = 0x88;
    else if (sNum1 == "9") bNumber = 0x89;

    else if (sNum1 == "외교") bNumber = 0x98;

    else if (sNum1 == "") bNumber = 0x99;

    if (bCommercial && bNumber < 0x17) bNumber += 0x40;  ///'영'자

    return bNumber;

}

QString CCUComm::GetRegion_byCode(quint8 bNumber)
{
    QString sNum1 = "";

    if (bNumber == 0x00) sNum1 = "서울";
    else if (bNumber == 0x01) sNum1 = "부산";
    else if (bNumber == 0x02) sNum1 = "인천";
    else if (bNumber == 0x03) sNum1 = "대구";
    else if (bNumber == 0x04) sNum1 = "광주";
    else if (bNumber == 0x05) sNum1 = "대전";
    else if (bNumber == 0x06) sNum1 = "경기";
    else if (bNumber == 0x07) sNum1 = "강원";
    else if (bNumber == 0x08) sNum1 = "충북";
    else if (bNumber == 0x09) sNum1 = "충남";
    else if (bNumber == 0x10) sNum1 = "전북";
    else if (bNumber == 0x11) sNum1 = "전남";
    else if (bNumber == 0x12) sNum1 = "경북";
    else if (bNumber == 0x13) sNum1 = "경남";
    else if (bNumber == 0x14) sNum1 = "제주";
    else if (bNumber == 0x15) sNum1 = "울산";
    else if (bNumber == 0x16) sNum1 = "세종";

    else if (bNumber == 0x40) sNum1 = "영서울";
    else if (bNumber == 0x41) sNum1 = "영부산";
    else if (bNumber == 0x42) sNum1 = "영인천";
    else if (bNumber == 0x43) sNum1 = "영대구";
    else if (bNumber == 0x44) sNum1 = "영광주";
    else if (bNumber == 0x45) sNum1 = "영대전";
    else if (bNumber == 0x46) sNum1 = "영경기";
    else if (bNumber == 0x47) sNum1 = "영강원";
    else if (bNumber == 0x48) sNum1 = "영충북";
    else if (bNumber == 0x49) sNum1 = "영충남";
    else if (bNumber == 0x50) sNum1 = "영전북";
    else if (bNumber == 0x51) sNum1 = "영전남";
    else if (bNumber == 0x52) sNum1 = "영경북";
    else if (bNumber == 0x53) sNum1 = "영경남";
    else if (bNumber == 0x54) sNum1 = "영제주";
    else if (bNumber == 0x55) sNum1 = "영울산";
    else if (bNumber == 0x56) sNum1 = "영세종";

    else if (bNumber == 0x80) sNum1 = "0";
    else if (bNumber == 0x81) sNum1 = "1";
    else if (bNumber == 0x82) sNum1 = "2";
    else if (bNumber == 0x83) sNum1 = "3";
    else if (bNumber == 0x84) sNum1 = "4";
    else if (bNumber == 0x85) sNum1 = "5";
    else if (bNumber == 0x86) sNum1 = "6";
    else if (bNumber == 0x87) sNum1 = "7";
    else if (bNumber == 0x88) sNum1 = "8";
    else if (bNumber == 0x89) sNum1 = "9";

    else if (bNumber == 0x98) sNum1 = "외교";
    else if (bNumber == 0x99) sNum1 = "";

    return sNum1;
}

quint8 CCUComm::GetCarClass(QString sNum3)
{
    quint8 bNumber = 0x00;

    if (sNum3 == "가") bNumber = 0x00;
    else if (sNum3 == "나") bNumber = 0x01;
    else if (sNum3 == "다") bNumber = 0x02;
    else if (sNum3 == "라") bNumber = 0x03;
    else if (sNum3 == "마") bNumber = 0x04;
    else if (sNum3 == "바") bNumber = 0x05;
    else if (sNum3 == "사") bNumber = 0x06;
    else if (sNum3 == "아") bNumber = 0x07;
    else if (sNum3 == "자") bNumber = 0x08;
    else if (sNum3 == "차") bNumber = 0x09;

    else if (sNum3 == "카") bNumber = 0x10;
    else if (sNum3 == "타") bNumber = 0x11;
    else if (sNum3 == "파") bNumber = 0x12;
    else if (sNum3 == "하") bNumber = 0x13;
    else if (sNum3 == "거") bNumber = 0x14;
    else if (sNum3 == "너") bNumber = 0x15;
    else if (sNum3 == "더") bNumber = 0x16;
    else if (sNum3 == "러") bNumber = 0x17;
    else if (sNum3 == "머") bNumber = 0x18;
    else if (sNum3 == "버") bNumber = 0x19;

    else if (sNum3 == "서") bNumber = 0x20;
    else if (sNum3 == "어") bNumber = 0x21;
    else if (sNum3 == "저") bNumber = 0x22;
    else if (sNum3 == "처") bNumber = 0x23;
    else if (sNum3 == "커") bNumber = 0x24;
    else if (sNum3 == "터") bNumber = 0x25;
    else if (sNum3 == "퍼") bNumber = 0x26;
    else if (sNum3 == "허") bNumber = 0x27;
    else if (sNum3 == "고") bNumber = 0x28;
    else if (sNum3 == "노") bNumber = 0x29;

    else if (sNum3 == "도") bNumber = 0x30;
    else if (sNum3 == "로") bNumber = 0x31;
    else if (sNum3 == "모") bNumber = 0x32;
    else if (sNum3 == "보") bNumber = 0x33;
    else if (sNum3 == "소") bNumber = 0x34;
    else if (sNum3 == "오") bNumber = 0x35;
    else if (sNum3 == "조") bNumber = 0x36;
    else if (sNum3 == "초") bNumber = 0x37;
    else if (sNum3 == "코") bNumber = 0x38;
    else if (sNum3 == "토") bNumber = 0x39;

    else if (sNum3 == "포") bNumber = 0x40;
    else if (sNum3 == "호") bNumber = 0x41;
    else if (sNum3 == "구") bNumber = 0x42;
    else if (sNum3 == "누") bNumber = 0x43;
    else if (sNum3 == "두") bNumber = 0x44;
    else if (sNum3 == "루") bNumber = 0x45;
    else if (sNum3 == "무") bNumber = 0x46;
    else if (sNum3 == "부") bNumber = 0x47;
    else if (sNum3 == "수") bNumber = 0x48;
    else if (sNum3 == "우") bNumber = 0x49;

    else if (sNum3 == "주") bNumber = 0x50;
    else if (sNum3 == "추") bNumber = 0x51;
    else if (sNum3 == "쿠") bNumber = 0x52;
    else if (sNum3 == "투") bNumber = 0x53;
    else if (sNum3 == "푸") bNumber = 0x54;
    else if (sNum3 == "후") bNumber = 0x55;
    else if (sNum3 == "그") bNumber = 0x56;
    else if (sNum3 == "느") bNumber = 0x57;
    else if (sNum3 == "드") bNumber = 0x58;
    else if (sNum3 == "르") bNumber = 0x59;

    else if (sNum3 == "므") bNumber = 0x60;
    else if (sNum3 == "브") bNumber = 0x61;
    else if (sNum3 == "스") bNumber = 0x62;
    else if (sNum3 == "으") bNumber = 0x63;
    else if (sNum3 == "즈") bNumber = 0x64;
    else if (sNum3 == "츠") bNumber = 0x65;
    else if (sNum3 == "크") bNumber = 0x66;
    else if (sNum3 == "트") bNumber = 0x67;
    else if (sNum3 == "프") bNumber = 0x68;
    else if (sNum3 == "흐") bNumber = 0x69;

    else if (sNum3 == "기") bNumber = 0x70;
    else if (sNum3 == "니") bNumber = 0x71;
    else if (sNum3 == "디") bNumber = 0x72;
    else if (sNum3 == "리") bNumber = 0x73;
    else if (sNum3 == "미") bNumber = 0x74;
    else if (sNum3 == "비") bNumber = 0x75;
    else if (sNum3 == "시") bNumber = 0x76;
    else if (sNum3 == "이") bNumber = 0x77;
    else if (sNum3 == "지") bNumber = 0x78;
    else if (sNum3 == "치") bNumber = 0x79;

    else if (sNum3 == "키") bNumber = 0x80;
    else if (sNum3 == "티") bNumber = 0x81;
    else if (sNum3 == "피") bNumber = 0x82;
    else if (sNum3 == "히") bNumber = 0x83;
    else if (sNum3 == "육") bNumber = 0x84;
    else if (sNum3 == "해") bNumber = 0x85;
    else if (sNum3 == "공") bNumber = 0x86;
    else if (sNum3 == "국") bNumber = 0x87;
    else if (sNum3 == "합") bNumber = 0x88;
    else if (sNum3 == "-") bNumber = 0x89;

    else if (sNum3 == "배") bNumber = 0x90;
    else if (sNum3 == "외") bNumber = 0x98;

    return bNumber;
}

QString CCUComm::GetCarClass_byCode(quint8 bNumber)
{
    QString sNum3 = "";

    if (bNumber == 0x00) sNum3 = "가";
    else if (bNumber == 0x01) sNum3 = "나";
    else if (bNumber == 0x02) sNum3 = "다";
    else if (bNumber == 0x03) sNum3 = "라";
    else if (bNumber == 0x04) sNum3 = "마";
    else if (bNumber == 0x05) sNum3 = "바";
    else if (bNumber == 0x06) sNum3 = "사";
    else if (bNumber == 0x07) sNum3 = "아";
    else if (bNumber == 0x08) sNum3 = "자";
    else if (bNumber == 0x09) sNum3 = "차";

    else if (bNumber == 0x10) sNum3 = "카";
    else if (bNumber == 0x11) sNum3 = "타";
    else if (bNumber == 0x12) sNum3 = "파";
    else if (bNumber == 0x13) sNum3 = "하";
    else if (bNumber == 0x14) sNum3 = "거";
    else if (bNumber == 0x15) sNum3 = "너";
    else if (bNumber == 0x16) sNum3 = "더";
    else if (bNumber == 0x17) sNum3 = "러";
    else if (bNumber == 0x18) sNum3 = "머";
    else if (bNumber == 0x19) sNum3 = "버";

    else if (bNumber == 0x20) sNum3 = "서";
    else if (bNumber == 0x21) sNum3 = "어";
    else if (bNumber == 0x22) sNum3 = "저";
    else if (bNumber == 0x23) sNum3 = "처";
    else if (bNumber == 0x24) sNum3 = "커";
    else if (bNumber == 0x25) sNum3 = "터";
    else if (bNumber == 0x26) sNum3 = "퍼";
    else if (bNumber == 0x27) sNum3 = "허";
    else if (bNumber == 0x28) sNum3 = "고";
    else if (bNumber == 0x29) sNum3 = "노";

    else if (bNumber == 0x30) sNum3 = "도";
    else if (bNumber == 0x31) sNum3 = "로";
    else if (bNumber == 0x32) sNum3 = "모";
    else if (bNumber == 0x33) sNum3 = "보";
    else if (bNumber == 0x34) sNum3 = "소";
    else if (bNumber == 0x35) sNum3 = "오";
    else if (bNumber == 0x36) sNum3 = "조";
    else if (bNumber == 0x37) sNum3 = "초";
    else if (bNumber == 0x38) sNum3 = "코";
    else if (bNumber == 0x39) sNum3 = "토";

    else if (bNumber == 0x40) sNum3 = "포";
    else if (bNumber == 0x41) sNum3 = "호";
    else if (bNumber == 0x42) sNum3 = "구";
    else if (bNumber == 0x43) sNum3 = "누";
    else if (bNumber == 0x44) sNum3 = "두";
    else if (bNumber == 0x45) sNum3 = "루";
    else if (bNumber == 0x46) sNum3 = "무";
    else if (bNumber == 0x47) sNum3 = "부";
    else if (bNumber == 0x48) sNum3 = "수";
    else if (bNumber == 0x49) sNum3 = "우";

    else if (bNumber == 0x50) sNum3 = "주";
    else if (bNumber == 0x51) sNum3 = "추";
    else if (bNumber == 0x52) sNum3 = "쿠";
    else if (bNumber == 0x53) sNum3 = "투";
    else if (bNumber == 0x54) sNum3 = "푸";
    else if (bNumber == 0x55) sNum3 = "후";
    else if (bNumber == 0x56) sNum3 = "그";
    else if (bNumber == 0x57) sNum3 = "느";
    else if (bNumber == 0x58) sNum3 = "드";
    else if (bNumber == 0x59) sNum3 = "르";

    else if (bNumber == 0x60) sNum3 = "므";
    else if (bNumber == 0x61) sNum3 = "브";
    else if (bNumber == 0x62) sNum3 = "스";
    else if (bNumber == 0x63) sNum3 = "으";
    else if (bNumber == 0x64) sNum3 = "즈";
    else if (bNumber == 0x65) sNum3 = "츠";
    else if (bNumber == 0x66) sNum3 = "크";
    else if (bNumber == 0x67) sNum3 = "트";
    else if (bNumber == 0x69) sNum3 = "흐";
    else if (bNumber == 0x70) sNum3 = "기";
    else if (bNumber == 0x71) sNum3 = "니";
    else if (bNumber == 0x72) sNum3 = "디";
    else if (bNumber == 0x73) sNum3 = "리";
    else if (bNumber == 0x74) sNum3 = "미";
    else if (bNumber == 0x75) sNum3 = "비";
    else if (bNumber == 0x76) sNum3 = "시";
    else if (bNumber == 0x77) sNum3 = "이";
    else if (bNumber == 0x78) sNum3 = "지";
    else if (bNumber == 0x79) sNum3 = "치";

    else if (bNumber == 0x80) sNum3 = "키";
    else if (bNumber == 0x81) sNum3 = "티";
    else if (bNumber == 0x82) sNum3 = "피";
    else if (bNumber == 0x83) sNum3 = "히";
    else if (bNumber == 0x84) sNum3 = "육";
    else if (bNumber == 0x85) sNum3 = "해";
    else if (bNumber == 0x86) sNum3 = "공";
    else if (bNumber == 0x87) sNum3 = "국";
    else if (bNumber == 0x88) sNum3 = "합";
    else if (bNumber == 0x89) sNum3 = "-";

    else if (bNumber == 0x90) sNum3 = "배";
    else if (bNumber == 0x98) sNum3 = "";// "외";

    return sNum3;
}

//void CCUComm::readData()
//{
//    //sdw 2016/10/31
//    qint64 count = serial.bytesAvailable();

//    if( count > 0)
//    {
//        //qDebug() << QString("spccommunication ReadData : %1").arg(count);
//        //sdw //2016/09/21  realloc error로 변경함.
//        QByteArray data;
//        data.append(serial.readAll());
//       //sdw 2016/09/12 insert mutex
//        rmutex.lock();
//        InputData.append(data);
//        rmutex.unlock();
////        if(m_loglevel >= LOG_DEBUG)
// //       {
//            QString logstr = QString("%1 ReadData-len(%2): %3")
//                    .arg(m_serialport).arg(data.size()).arg(QString(data.toHex()));
//            qDebug() << logstr;
// //           log->write(logstr,LOG_DEBUG);
////        }
//    }
//    else
//    {
//        QString logstr = QString("%1 ReadData Error(%2)")
//                .arg(m_serialport).arg(count);
//        qDebug() <<logstr;
////        log->write(logstr,LOG_ERR);
//    }
//}

//void CCUComm::handleError(QSerialPort::SerialPortError error)
//{
//    if(error != QSerialPort::SerialPortError::NoError)
//    {
//        QString logstr = QString("Serial Error(CCU ch:%1) : %2").arg(m_channel).arg(error);
//        log->write(logstr,LOG_ERR);

//        if(error == QSerialPort::SerialPortError::ResourceError) SerialDeviceSetting();
//    }
//}


