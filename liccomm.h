#ifndef LICCOMM_H
#define LICCOMM_H

#include <QObject>
#include <QThread>
#include <QtSerialPort/QSerialPort>
#include <QQueue>
#include <QDebug>
#include <QDateTime>
#include <QMutex>

#include "syslogger.h"
#include "logdlg.h"

class LIC_SettingData_Info
{
public:
    quint8 PAN;
    quint8 TILT;
    quint8 Zoom;
    quint8 Focus;
    quint8 Iris;
    quint8 CDS;
};

class LIC_Status_Info
{
public:
     quint8 Reset;
     quint16 PAN;
     quint16 TILT;
     quint16 Zoom;
     quint16 Focus;
     quint16 Iris;
     quint16 CDS;
     quint8 Temperature;
     quint8 Humidity;
     quint8 FAN;
     quint8 HEATER;
     quint8 DoorStatus;

};

class LIC_Light_Info
{
public:
    quint8 Status;
    quint8 ONOFF;
    quint8 Percent;
};




class LICComm : public QThread
{
    Q_OBJECT
public:
    enum SerialState
    {
        OPEN = 0x0000,
        OPENFAIL = 0x0001,
        PORTLOCKED = 0x0002,
        CLOSE = 0x0004
    };
public:
    explicit LICComm(int channel, int loglevel = LOG_INFO,QThread *parent = nullptr);
    ~LICComm();
    void stop();
    void run();
    int OpenSerial();
    int OpenSerial(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits
                   ,QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    int OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits
                   ,QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    void CloseSerial();
    void Parsing(QByteArray &data);
    void ProcessCommand(quint8 *cmddata,int cmdlen);    
    QString GetErrorCode(quint8 err);
    QByteArray Minus0x10(QByteArray rxdata);
    void SerialDeviceSetting();
    void SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits
                             , QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    void writeData(QByteArray data);

    QByteArray MakeSendPacket(quint8 Opcode, QByteArray Data = NULL);
    QByteArray Crc16(QByteArray data);
    QByteArray Uint16toArr(quint16 number);
    quint32 ArrtoUint(QByteArray bdata);
    void SettingREQ_Send();
    void StatusREQ_Send();
    void LightREQ_Send();
    void SettingTrans_Send(quint8 pan, quint8 tilt, quint8 zoom, quint8 focus, quint8 iris, quint8 cds, quint8 light);
    void Preset_Control_Send(quint16 ipan, quint16 itilt, quint16 izoom, quint16 ifocus, quint16 iiris);
    void Time_control_Send(quint8 type, quint8 direction, quint16 msec);
    void Soft_Reset_Send(quint8 data);
    void ACK_Send();
    void NAK_Send(quint8 errcode);
    void SetLight(bool blight);

signals:
    void sigReceiveHandler(int channel,quint8 Cmd,QString data );

public slots:
   // void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
   bool ThreadStop;
   QSerialPort serial;
   QQueue<QByteArray> InputData;
   quint8 pdata[544]; //512+ 32 = 544
   int pdata_endindex;
   //sdw 2016/09/12  insert mutex
   QMutex rmutex;
   QMutex wmutex;

public:
   int m_state;
   int m_channel;
   bool m_breconnect;
   QString m_serialport;
   int m_baudrate;
   QSerialPort::DataBits m_databits;
   QSerialPort::StopBits m_stopbits;
   QSerialPort::Parity m_parity;
   QSerialPort::FlowControl m_flowcontrol;

   LIC_SettingData_Info SettingINFO;
   LIC_Status_Info StatusINFO;
   LIC_Light_Info lightINFO;

   LogDlg *plogdlg;
   int m_loglevel;
   Syslogger *log;

   int m_loopcount;

   enum ProtocolType
   {
       DLE = 0x10,
       STX = 0x02,
       ETX = 0x03,
   };

   enum LIC_OPCODE
   {
       CMD_SettingREQ = 0x81,
       CMD_StatusREQ = 0x82,
       CMD_LightREQ = 0x83,
       CMD_SettingTrans = 0x91,
       CMD_Preset_Control = 0x92,
       CMD_Time_Control = 0x93,
       CMD_Soft_Reset = 0x94,
       CMD_Trigger = 0x84,
       CMD_ACK = 0x06,
       CMD_NAK = 0x15,
       //inpeg add
       CMD_Connected = 0x00,
       CMD_Disconnected = 0x01,



       ERR_E1 = 0xE1,
       ERR_E2 = 0xE2,
       ERR_E3 = 0xE3,
       ERR_E4 = 0xE4,
       ERR_FF = 0xFF,

       SUCCESS = 0x00,

       RESET_LIC = 0x00,
       RESET_CAMERA = 0x01,
       RESET_LOOP = 0x02
   };


};

#endif // LICCOMM_H
