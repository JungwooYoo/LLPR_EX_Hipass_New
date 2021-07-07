#include "liccomm.h"
#include <QFileInfo>

LICComm::LICComm(int channel, int loglevel ,QThread *parent) : QThread(parent)
{
    ThreadStop=false;
    pdata_endindex = 0;

    m_channel = channel;
    log = new Syslogger(this,"LICComm",true,loglevel);
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

LICComm::~LICComm()
{
    disconnect(&serial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(handleError(QSerialPort::SerialPortError)));
    //disconnect(&serial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void LICComm::stop()
{
    ThreadStop = true;
}

void LICComm::run()
{
    int datacheckcount=0;
    //thread
    ThreadStop=false;
    m_loopcount = 0;

    QString logstr = QString("LIC Thread Start");
    qDebug() << logstr;
    log->write(logstr,LOG_ERR);

    while (!ThreadStop)
    {
        try
        {
            //check Input Data
            qint64 count = serial.bytesAvailable();
            //if(!InputData.isEmpty())
            if( count > 0)
            {
                QByteArray data;
                data.append(serial.readAll());
                if( m_loglevel >= LOG_DEBUG )
                {
                    logstr =  QString("RX_ORG(LIC/ch%1) : %2").arg(m_channel).arg(QString(data.toHex()));
                    log->write(logstr,LOG_DEBUG);   qDebug() << logstr;
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
            logstr = QString("--------- Error LIC Thread(ch%1) -----------").arg(m_channel);
            log->write(logstr,LOG_ERR);            qDebug() << logstr;
        }
        msleep(10);

        m_loopcount++;

    }

    CloseSerial();

    logstr = QString("LIC Thread Stop");
    qDebug() << logstr;
    log->write(logstr,LOG_ERR);
}

int LICComm::OpenSerial()
{
    return OpenSerial(m_serialport,m_baudrate,m_databits,m_stopbits,m_parity,m_flowcontrol);
}

int LICComm::OpenSerial(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
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

int LICComm::OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
{
    int iret = -1;
    try{
        if( serial.isOpen())
        {
            qDebug() << "LIC is already Open";
            return iret;
        }


        QString lockfile = QString("/var/lock/LCK..%1").arg(m_serialport);
        QFileInfo fi(lockfile);
        //sdw 2016/11/11
        if( fi.exists() )
        {
            //qDebug() << QString("LIC port is in use elsewhere.");
            QString strcmd = QString("rm -rf %1").arg(lockfile);
            system(strcmd.toStdString().c_str());
            //return iret;
        }

        QString devicename = QString("/dev/%1").arg(portName);
        serial.setPortName(devicename);
        if (serial.open(QIODevice::ReadWrite)) {
            if (serial.setBaudRate(baudrate) && serial.setDataBits(databits)
                        && serial.setParity(parity) && serial.setStopBits(stopbits)
                        && serial.setFlowControl(flowcontrol))
            {
                qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "[OpenSerial]LIC Port Open";
    //            if(plogdlg != NULL)
    //                plogdlg->logappend(logdlg::logspc,"[OpenSPC]SPC Port Open");
                m_state = OPEN;
                iret = 0;
                emit sigReceiveHandler(m_channel,CMD_Connected,"");

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

void LICComm::CloseSerial()
{
    serial.close();
    QString logstr = "[CloseSerial]LIC Port Close";
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << logstr;
    //if(plogdlg != NULL) plogdlg->logappend(logdlg::logspc,logstr);
    emit sigReceiveHandler(m_channel,CMD_Disconnected,"");

    m_state |= CLOSE;
}

void LICComm::Parsing(QByteArray &data)
{
    int data_count = data.size();
    memcpy(&pdata[pdata_endindex],data.data(),sizeof(unsigned char)*data_count);
    pdata_endindex += data_count;

    if(pdata_endindex > 255)
    {
        qDebug() << QString("Receive Data overflow(LIC)");
        memcpy(&pdata[0],data.data(),sizeof(unsigned char)*data_count);
        pdata_endindex = data_count;
    }

    int stxindex = 0;
    while(stxindex < pdata_endindex)
    {
        if( pdata[stxindex] == DLE && pdata[stxindex + 1] == STX )
        {
            int etxindex = stxindex + 2;
            while(etxindex < pdata_endindex)
            {
                if( pdata[etxindex] == DLE && pdata[etxindex - 1] == DLE )
                    etxindex += 2;
                else if( pdata[etxindex] == ETX && pdata[etxindex - 1] == DLE)
                { //process cmd
                    int cmdlen = etxindex - stxindex + 3; //crc 1byte [+]
                    quint8 cmddata[cmdlen];
                    memcpy(cmddata,&pdata[stxindex],sizeof(quint8)*cmdlen);
                    ProcessCommand(cmddata,cmdlen);
                    stxindex = etxindex + 3;
                    break;
                }
                else
                    etxindex++;
            }
            if( etxindex >= pdata_endindex)
            { //Save rest data
                int rest_count;
                rest_count = pdata_endindex - stxindex;
                quint8 data_p[rest_count];
                memcpy(data_p,&pdata[stxindex],sizeof(quint8) * rest_count);
                memcpy(pdata, data_p,sizeof(quint8) * rest_count);
                pdata_endindex = rest_count;
                return;;
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

void LICComm::ProcessCommand(quint8 *cmddata, int cmdlen)
{
    QString logstr;
    try
    {
        QByteArray qcommandData = QByteArray((const char*)cmddata,cmdlen);
        qDebug() << QString("RX(LIC) : %1").arg(QString(qcommandData.toHex()));

        QByteArray Command = Minus0x10(qcommandData);
        if(Command.isNull() || Command.isEmpty())
        {
            qDebug() << QString("ProcessCommand-Minus0x10 is zero");
            return;
        }

        //CRC ignore

        quint8 Opcode = (quint8)Command[2];

        if( Opcode == CMD_SettingREQ)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
            {
                strCommand = QString("%1,%2,%3,%4,%5,%6").arg((int)Command[4]).arg((quint8)Command[5]).arg((quint8)Command[6])
                        .arg((quint8)Command[7]).arg((quint8)Command[8]).arg((quint8)Command[9]);
                SettingINFO.PAN = (quint8)Command[4];
                SettingINFO.TILT = (quint8)Command[5];
                SettingINFO.Zoom = (quint8)Command[6];
                SettingINFO.Focus = (quint8)Command[7];
                SettingINFO.Iris = (quint8)Command[8];
                SettingINFO.CDS = (quint8)Command[9];
            }
            emit  sigReceiveHandler(m_channel,CMD_SettingREQ,strCommand);
            logstr = QString("CMD_SettingREQ REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_StatusREQ)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
            {
                StatusINFO.Reset = (quint8)Command[4];
                StatusINFO.PAN = (quint16)(ArrtoUint(Command.mid(5,2)));
                StatusINFO.TILT = (quint16)(ArrtoUint(Command.mid(7,2)));
                StatusINFO.Zoom = (quint16)(ArrtoUint(Command.mid(9,2)));
                StatusINFO.Focus = (quint16)(ArrtoUint(Command.mid(11,2)));
                StatusINFO.Iris = (quint16)(ArrtoUint(Command.mid(13,2)));
                StatusINFO.CDS = (quint16)(ArrtoUint(Command.mid(15,2)));
                StatusINFO.Temperature = (quint8)Command[17];
                StatusINFO.Humidity = (quint8)Command[18];
                StatusINFO.FAN = (quint8)Command[19];
                StatusINFO.HEATER = (quint8)Command[20];
                StatusINFO.DoorStatus = (quint8)Command[21];

                strCommand = QString("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12")
                        .arg(StatusINFO.Reset).arg(StatusINFO.PAN).arg(StatusINFO.TILT)
                        .arg(StatusINFO.Zoom).arg(StatusINFO.Focus).arg(StatusINFO.Iris)
                        .arg(StatusINFO.CDS).arg(StatusINFO.Temperature).arg(StatusINFO.Humidity)
                        .arg(StatusINFO.FAN).arg(StatusINFO.HEATER).arg(StatusINFO.DoorStatus);
            }
            emit  sigReceiveHandler(m_channel,CMD_StatusREQ,strCommand);
            logstr = QString("CMD_StatusREQ REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_LightREQ)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
            {
                lightINFO.Status = (quint8)Command[4];
                lightINFO.ONOFF = (quint8)Command[5];
                lightINFO.Percent = (quint8)Command[6];

                strCommand = QString("%1,%2,%3")
                        .arg(lightINFO.Status).arg(lightINFO.ONOFF).arg(lightINFO.Percent);

            }
            emit  sigReceiveHandler(m_channel,CMD_LightREQ,strCommand);
            logstr = QString("CMD_LightREQ REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_SettingTrans)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
                strCommand = QString("%1,%2").arg((quint8)Command[4]).arg((quint8)Command[5]);

            emit  sigReceiveHandler(m_channel,CMD_SettingTrans,strCommand);
            logstr = QString("CMD_SettingTrans REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_Preset_Control)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";

            emit  sigReceiveHandler(m_channel,CMD_Preset_Control,strCommand);
            logstr = QString("CMD_Preset_Control REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_Soft_Reset)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
                 strCommand = QString("%1,%2").arg((quint8)Command[4]).arg((quint8)Command[5]);

            emit  sigReceiveHandler(m_channel,CMD_Soft_Reset,strCommand);
            logstr = QString("CMD_Soft_Reset REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else if( Opcode == CMD_Trigger)
        {
            QString strCommand = "";
            if(Command[4] == CMD_NAK) strCommand = QString("NAK : %1").arg(GetErrorCode(Command[5]));
            else if( Command[4] == CMD_ACK) strCommand = "ACK";
            else
                strCommand = QString("%1,%2").arg((quint8)Command[4]).arg((quint8)Command[5]);

            emit  sigReceiveHandler(m_channel,CMD_Trigger,strCommand);
            logstr = QString("CMD_Trigger REP : %1").arg(strCommand);
            qDebug() << logstr;
        }
        else
        {
            qDebug() << QString("LIC Command is Unknown");
        }

    }
    catch( ... )
    {
        qDebug() << QString("ProcessCommand Exception");
    }

}

QString LICComm::GetErrorCode(quint8 err)
{
    QString sRep = "";
   if (err == 0x00) sRep = "정상";
   else if (err == 0xE1) sRep = "알수 없는 명령";
   else if (err == 0xE2) sRep = "요구하지 않은 데이터";
   else if (err == 0xE3) sRep = "CRC 오류";
   else if (err == 0xE4) sRep = "Packet Lenght 오류";
   else if (err == 0xFF) sRep = "기타 정의되지 않은 오류";

   return sRep;
}


QByteArray LICComm::Minus0x10(QByteArray rxdata)
{
    QByteArray processdata;
    try
    {
        int i10count = 0;
        int i10checkindex = rxdata.length() - 4;
        int i10checkstart = 4;

        for( int i = i10checkstart; i < i10checkindex - 1; i++)
        {
            if( rxdata[i] == DLE && rxdata[i + 1] == DLE)
            {
                i10count++;
                i++;
            }
        }

        processdata = rxdata.mid(0,4);
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
        qDebug() << QString("Minus0x10(LIC) Exception");
        processdata.clear();
    }
    return processdata;

}

void LICComm::SerialDeviceSetting()
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

void LICComm::SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits, QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol)
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


void LICComm::writeData(QByteArray data)
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

QByteArray LICComm::MakeSendPacket(quint8 Opcode, QByteArray Data)
{
    QByteArray SendData;
    QByteArray repArr;

    try
    {
        int datalen = 0;
        if( !Data.isNull() && !Data.isEmpty() ) datalen = Data.length();

        SendData.append((quint8)DLE);
        SendData.append((quint8)STX);
        //crc start
        SendData.append(Opcode);
        SendData.append((quint8)datalen);
        if( datalen > 0)
            SendData.append(Data);
        SendData.append((quint8)DLE);
        SendData.append((quint8)ETX);

        QByteArray crcdata = SendData.mid(2, 2 + datalen);
        QByteArray crc = Crc16(crcdata);
        if(crc.length() != 2 )
        {
            SendData.append(2,(char)0x00);
        }
        else SendData.append(crc);

        int i10Count = 0;
        int senddatalen = SendData.length() - 4;
        for( int i=4; i < senddatalen; i++)
        {
            if(SendData[i] == (quint8)DLE) i10Count++;
        }

        repArr.append(SendData.mid(0,4));
        for( int i = 4; i < senddatalen; i++)
        {
            if(SendData[i] == (quint8)DLE)
                repArr.append(2,SendData[i]);
            else
                repArr.append(SendData[i]);
        }
        repArr.append(SendData.mid(senddatalen,4));

        qDebug() << QString("TX(LIC) : %1").arg(QString(QString(repArr.toHex())));

    }
    catch( ... )
    {
        qDebug() << QString("MakeSendPacket Exception");
        repArr.clear();
    }

    return repArr;
}

QByteArray LICComm::Crc16(QByteArray data)
{
    QByteArray bcrc;
    try
    {
        quint16 wCrc = 0xFFFF;
        quint16 wCh;
        int datalen = data.length();
        for (int i = 0; i < datalen; i++)
        {
            wCh = data[i];
            for (int j = 0; j < 8; j++)
            {
                if (((wCrc ^ wCh) & 0x0001) != 0)
                {
                    wCrc = (quint16)((wCrc >> 1) ^ 0xA001);
                }
                else
                {
                    wCrc >>= 1;
                }
                wCh >>= 1;
            }
        }
        bcrc.append((quint8)(wCrc>>8));
        bcrc.append((quint8)(wCrc&0x00FF));
    }
    catch( ... )
    {
        qDebug() << QString("Crc16 Exception");
        bcrc.clear();
    }
    return bcrc;
}

QByteArray LICComm::Uint16toArr(quint16 number)
{
    QByteArray conbytearr;
    conbytearr.append((quint8)(number >> 8));
    conbytearr.append((quint8)(number & 0x00FF));
    return conbytearr;

}

quint32 LICComm::ArrtoUint(QByteArray bdata)
{//bigendian
    int count = bdata.length();
    quint32 iret = 0;

    for(int index=0; index < count; index++)
    {
        if(index != 0 ) iret <<= 8;;
        iret += (quint8)bdata[index];
    }
    return iret;

}

void LICComm::SettingREQ_Send()
{
    QByteArray data(1,(char)0x00);
    QByteArray senddata = MakeSendPacket(CMD_SettingREQ,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("SettingREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("SettingREQ Send");
}

void LICComm::StatusREQ_Send()
{
    QByteArray data(1,(char)0x00);
    QByteArray senddata = MakeSendPacket(CMD_StatusREQ,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("StatusREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("StatusREQ Send");
}

void LICComm::LightREQ_Send()
{
    QByteArray data(1,(char)0x00);
    QByteArray senddata = MakeSendPacket(CMD_LightREQ,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("LightREQ_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("LightREQ Send");
}

void LICComm::SettingTrans_Send(quint8 pan, quint8 tilt, quint8 zoom, quint8 focus, quint8 iris, quint8 cds, quint8 light)
{
    QByteArray data;
    data.append(pan);
    data.append(tilt);
    data.append(zoom);
    data.append(focus);
    data.append(iris);
    data.append(cds);
    data.append(light);

    QByteArray senddata = MakeSendPacket(CMD_SettingTrans,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("SettingTrans_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("SettingTrans Send");
}

void LICComm::Preset_Control_Send(quint16 ipan, quint16 itilt, quint16 izoom, quint16 ifocus, quint16 iiris)
{
    QByteArray data;
    data.append(Uint16toArr(ipan));
    data.append(Uint16toArr(itilt));
    data.append(Uint16toArr(izoom));
    data.append(Uint16toArr(ifocus));
    data.append(Uint16toArr(iiris));

    QByteArray senddata = MakeSendPacket(CMD_Preset_Control,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("Prset_Control_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("Prset_Control Send");
}

//type : 0-pan, 1-tilt, 2-zoom, 3-focus, 4-iris
//direction : zoom-0:tele,1:wide  , focus-0:near,1:far, iris-0:open,1:close
void LICComm::Time_control_Send(quint8 type, quint8 direction, quint16 msec)
{
    QByteArray data;
    data.append(type);
    data.append(direction);
    data.append(Uint16toArr(msec));


    QByteArray senddata = MakeSendPacket(CMD_Time_Control,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("Time_control_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("Time_control Send");
}

void LICComm::Soft_Reset_Send(quint8 data)
{
    //data : 0-lic reset , 1-camera reset , 2-loopdetect reset
    QByteArray bdata(1,data);
    QByteArray senddata = MakeSendPacket(CMD_Soft_Reset,bdata);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("Soft_Reset_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("Soft_Reset Send");
}

void LICComm::ACK_Send()
{
    QByteArray data(1,(char)0x00);
    QByteArray senddata = MakeSendPacket(CMD_ACK,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("ACK_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("ACK Send");
}

void LICComm::NAK_Send(quint8 errcode)
{
    QByteArray data(1,errcode);
    QByteArray senddata = MakeSendPacket(CMD_NAK,data);
    if(senddata.isNull() || senddata.isEmpty())
    {
        qDebug() << QString("NAK_Send is NULL/Empty");
        return;
    }
    writeData(senddata);
    qDebug() << QString("NAK Send");
}

void LICComm::SetLight(bool blight)
{
    quint8 lightstatus = blight ? (quint8)0x01 : (quint8)0x00; // 0x00:Off, 0x01:On
    // pan/tilt/zoom/focus  direction 0x00,0x01
    SettingTrans_Send(SettingINFO.PAN,SettingINFO.TILT
                      ,SettingINFO.Zoom,SettingINFO.Focus
                      ,SettingINFO.Iris,SettingINFO.CDS, lightstatus);
    msleep(20);
    LightREQ_Send();
    qDebug() << QString("SetLight(%1)").arg(blight ? "ON" : "OFF");
}

//void LICComm::readData()
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

void LICComm::handleError(QSerialPort::SerialPortError error)
{
    if(error != QSerialPort::SerialPortError::NoError)
    {
        QString logstr = QString("Serial Error(LIC ch:%1) : %2").arg(m_channel).arg(error);
        log->write(logstr,LOG_ERR);
    }
}




