#include "tgmcucomm.h"
#include <QFileInfo>

TGMCUComm::TGMCUComm(int loglevel, QThread *parent) : QThread(parent)
{
    ThreadStop=false;
    pdata_endindex = 0;    

    log = new Syslogger(this,"tgmcu",true,loglevel);
    m_loglevel = loglevel;

    m_state = CLOSE;
    m_breconnect = false;
    m_serialport = "";
    m_baudrate = 115200;
    m_databits = QSerialPort::Data8;
    m_stopbits = QSerialPort::OneStop;
    m_parity = QSerialPort::NoParity;
    m_flowcontrol = QSerialPort::NoFlowControl;

    connect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    //connect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));

    //start();
}

TGMCUComm::~TGMCUComm()
{
    disconnect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    //disconnect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void TGMCUComm::SetLogLevel(int loglevel)
{

}

void TGMCUComm::stop()
{
    ThreadStop = true;
}

void TGMCUComm::run()
{
    int datacheckcount=0;
    //thread
    ThreadStop=false;
    m_loopcount=0;

    QString logstr = QString("TGMCU Thread Start");
    qDebug() << logstr;
    log->write(logstr,LOG_ERR);

    while (!ThreadStop)
    {
        try
        {
            qint64 count = serial.bytesAvailable();
            //if(!InputData.isEmpty())
            if( count > 0)
            {
                QByteArray data;
                data.append(serial.readAll());
                if( m_loglevel >= LOG_DEBUG )
                {
                    logstr =  QString("RX_ORG(TGMCU) : %2").arg(QString(data.toHex()));
                    log->write(logstr,LOG_DEBUG);
                    qDebug() << logstr;
                }
                //sdw 2016/09/12 insert mutex
    //            rmutex.lock();
    //            QByteArray data = InputData.dequeue();
    //            rmutex.unlock();

                //emit SpcReadData(data);
                Parsing(data);
                //log->write(QString(data.toHex()),LOG_DEBUG);
                datacheckcount=0;
            }
            else
            {
                msleep(10);
                datacheckcount++;
                if(datacheckcount > 200) // no input data 2sec
                {
                    pdata_endindex = 0;
                    datacheckcount = 0;
                }
            }
        }
        catch( ... )
        {
            logstr = QString("--------- Error TGMCU Thread-----------");
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }

        msleep(10);
        m_loopcount++;

    }

    CloseSerial();

    logstr = QString("TGMCU Thread Stop");
    log->write(logstr,LOG_ERR);
    qDebug() << logstr;
}

int TGMCUComm::OpenSerial()
{
    return OpenSerial(m_serialport,m_baudrate,m_databits,m_stopbits,m_parity,m_flowcontrol);
}

int TGMCUComm::OpenSerial(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
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

int TGMCUComm::OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
{
    int iret = -1;
    try{
        if( serial.isOpen())
        {
            qDebug() << "TGMCU is already Open";
            return iret;
        }


        QString lockfile = QString("/var/lock/LCK..%1").arg(m_serialport);
        QFileInfo fi(lockfile);
        //sdw 2016/11/11
        if( fi.exists() )
        {
//            qDebug() << QString("TGMCU port is in use elsewhere.");
            QString strcmd = QString("rm -rf %1").arg(lockfile);
            system(strcmd.toStdString().c_str());
//            return iret;
        }

        QString devicename = QString("/dev/%1").arg(portName);
        serial.setPortName(devicename);
        if (serial.open(QIODevice::ReadWrite)) {
            if (serial.setBaudRate(baudrate) && serial.setDataBits(databits)
                        && serial.setParity(parity) && serial.setStopBits(stopbits)
                        && serial.setFlowControl(flowcontrol))
            {
                qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]TGMCU Port Open";
    //            if(plogdlg != NULL)
    //                plogdlg->logappend(logdlg::logspc,"[OpenSPC]SPC Port Open");
                m_state = OPEN;
                iret = 0;
                emit sigReceiveHandler(CMD_Connected,"");

            }
            else {
                 qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]" + serial.errorString();
    //             if(plogdlg != NULL)
    //                 plogdlg->logappend(logdlg::logspc,"[OpenSPC]" +serial.errorString());
                 m_state |= OPENFAIL;
                 //sdw 160912
                 //open error -> serial.close;
                 CloseSerial();
            }
        }
        else {

            qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]" + serial.errorString();
    //        if(plogdlg != NULL)
    //            plogdlg->logappend(logdlg::logspc,"[OpenSPC]" + serial.errorString());
            m_state |= OPENFAIL;
            //sdw 160912
            //open error -> serial.close;
            CloseSerial();
        }
    }
    catch( ...  )
    {
        qDebug() << QString("OpenSerial Exception");
        iret = -1;
    }

    return iret;
}

void TGMCUComm::CloseSerial()
{
    serial.close();
    QString logstr = "[CloseSPC]TGMCU Port Close";
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    //if(plogdlg != NULL) plogdlg->logappend(logdlg::logspc,logstr);
    emit sigReceiveHandler(CMD_Disconnected,"");

    m_state |= CLOSE;
}

void TGMCUComm::Parsing(QByteArray &data)
{
    int data_count = data.size();

    //쓰레기 데이터가 포함된 데이터가 버퍼를 초과하는 경우에 대해서 예외처리를 함.
    //TGMCU의 경우 4~10byte의 프로토콜로 255byte가 수신될 경우가 거의 없음.
    //예외발생 시 데이터 확인결과 앞의 데이터만 정상적인 데이터이며 나머지 데이터는 00h임
    if( data_count > 255)
    {
        QString logstr = QString("ORG Receive Data overflow(TGMCU) - %1(cut data : 255)").arg(data_count);
        qDebug() << logstr;
        log->write(logstr,LOG_ERR);
        data_count = 255;
    }

    memcpy(&pdata[pdata_endindex],data.data(),sizeof(unsigned char)*data_count);
    pdata_endindex += data_count;

//    QString logstr = QString("Receive Data(TGMCU) - %1").arg(pdata_endindex);
//    qDebug() << logstr;

    if(pdata_endindex > 255)
    {
        QString logstr = QString("Receive Data overflow(TGMCU) - %1").arg(pdata_endindex);
        qDebug() << logstr;
        log->write(logstr,LOG_ERR);
        memcpy(&pdata[0],data.data(),sizeof(unsigned char)*data_count);
        pdata_endindex = data_count;
    }

    int stxindex = 0;
    while( stxindex < pdata_endindex )
    {
        if( pdata[stxindex] == STX)
        {
            int etxindex = stxindex +1;
            while( etxindex < pdata_endindex)
            {
                if( pdata[etxindex] == ETX)
                { // 명령어 블럭에 대한 처리 후 남은 데이터부터 다시 시작(stx 검색)
                    int cmdlen = etxindex - stxindex + 1;
                    quint8 cmddata[cmdlen];
                    memcpy(cmddata,&pdata[stxindex],sizeof(quint8)*cmdlen);
                    ProcessCommand(cmddata,cmdlen);
                    stxindex = etxindex + 1;
                    break;
                }
                else
                    etxindex++;
            }
            if( etxindex >= pdata_endindex)
            {
                //Save rest data
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

void TGMCUComm::ProcessCommand(quint8 *cmddata, int cmdlen)
{
    QString logstr;
    try
    {
        QByteArray Command = QByteArray((const char*)cmddata,cmdlen);
        qDebug() << QString("RX(TGMCU) : %1").arg(QString(Command.toHex()));

        if( Command.length() < 3)
        {
            qDebug() << QString("Err Data Length is short : %1").arg(Command.length());
        }

        quint8 Opcode = (quint8)Command[1];

        if( Opcode == CMD_Trigger)
        {
            QString strCommand = QString("%1").arg((char)Command[2]);
            emit  sigReceiveHandler(CMD_Trigger,strCommand);
            logstr = QString("CMD_Trigger : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_COMTestREP)
        {
            emit  sigReceiveHandler(CMD_COMTestREP,"");
            logstr = QString("CMD_COMTestREP");
            qDebug() << logstr;
        }
        else if( Opcode == CMD_SlotLaneREP)
        {
            QString strCommand = QString("%1,%2").arg((char)Command[2]).arg((char)Command[3]);
            emit  sigReceiveHandler(CMD_SlotLaneREP,strCommand);
            logstr = QString("CMD_SlotLaneREP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_ACK)
        {
            emit  sigReceiveHandler(CMD_SlotLaneREP,"");
            logstr = QString("CMD_ACK");
            qDebug() << logstr;
        }
        else if( Opcode == CMD_NAK)
        {
            QString strCommand = QString("%1").arg((char)Command[2],0,16);
            emit  sigReceiveHandler(CMD_NAK,strCommand);
            logstr = QString("CMD_NAK : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else
        {
            qDebug() << QString("TGMCU Command is Unknown");
        }

    }
    catch( ... )
    {
        qDebug() << QString("ProcessCommand Exception");
    }
}

void TGMCUComm::SerialDeviceSetting()
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

void TGMCUComm::SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
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

void TGMCUComm::writeData(QByteArray data)
{
    //serial close -> reconnect
    if(m_breconnect && !serial.isOpen())
    {
        OpenSerial();
    }

    //sdw 2016/10/12
    wmutex.lock();
    //sdw 2016/09/12  Check serial open
    if(serial.isOpen())
    {
        serial.write(data);
        serial.waitForBytesWritten(1000);
//        if(m_loglevel >= LOG_DEBUG)
//        {
            QString logstr = QString("%1 writeData-len(%2): %3")
                    .arg(m_serialport).arg(data.size()).arg(QString(data.toHex()));
            qDebug() << logstr;
//            log->write(logstr,LOG_DEBUG);
//        }
    }
    else
    {
        QString logstr = QString("[writeData]%1 Port Close").arg(m_serialport);
        qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    }
    wmutex.unlock();
}

QByteArray TGMCUComm::MakeSendPacket(quint8 Opcode, QByteArray Data)
{
    QByteArray repArr;

    try
    {
        int datalen = 0;
        if( !Data.isNull() && !Data.isEmpty() ) datalen = Data.length();

        repArr.append((quint8)DLE);
        repArr.append((quint8)STX);
        repArr.append(Opcode);
        if( datalen > 0)
            repArr.append(Data);
        repArr.append((quint8)DLE);
        repArr.append((quint8)ETX);

        qDebug() << QString("TX(TGMCU) : %1").arg(QString(repArr.toHex()));

    }
    catch( ... )
    {
        qDebug() << QString("MakeSendPacket Exception");
        repArr.clear();
    }

    return repArr;
}

void TGMCUComm::COMTestREQ_Send()
{
    QByteArray senddata = MakeSendPacket(CMD_COMTestREQ,NULL);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("COMTestREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("COMTestREQ Send");
}

void TGMCUComm::TriggerREQ_Send(quint8 lane)
{
    QByteArray data(1,(char)lane);  //1lane -> 0x01, 2lan -> 0x02
    QByteArray senddata = MakeSendPacket(CMD_TriggerREQ,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("TriggerREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("TriggerREQ Send");
}

void TGMCUComm::SlotLaneREQ_Send()
{
    QByteArray senddata = MakeSendPacket(CMD_SlotLaneREQ,NULL);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("SlotLaneREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("SlotLaneREQ Send");
}

void TGMCUComm::ResetREQ_Send()
{
    QByteArray senddata = QString("RESET").toUtf8();
    writeData(senddata);
    qDebug() << QString("RESET Send");
}

//void TGMCUComm::readData()
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
//        InputData.enqueue(data);
//        rmutex.unlock();
// //       if(m_loglevel >= LOG_DEBUG)
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

void TGMCUComm::handleError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::SerialPortError::NoError)
    {
        QString logstr = QString("Serial Error(TGMCU) : %1").arg(error);
        log->write(logstr,LOG_ERR);
    }
}



