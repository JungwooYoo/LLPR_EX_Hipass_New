#ifndef TGMCUCOMM_H
#define TGMCUCOMM_H

#include <QObject>
#include <QThread>
#include <QtSerialPort/QSerialPort>
#include <QQueue>
#include <QDebug>
#include <QDateTime>
#include <QMutex>

#include "logdlg.h"
#include "syslogger.h"

class TGMCUComm : public QThread
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
    explicit TGMCUComm(int loglevel = LOG_INFO,QThread *parent = nullptr);
    ~TGMCUComm();
    void SetLogLevel(int loglevel);
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
    void SerialDeviceSetting();
    void SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits
                                 , QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    void writeData(QByteArray data);

    QByteArray MakeSendPacket(quint8 Opcode, QByteArray Data = NULL);
    void COMTestREQ_Send();
    void TriggerREQ_Send(quint8 lane);
    void SlotLaneREQ_Send();
    void ResetREQ_Send();


signals:
    void sigReceiveHandler(quint8 Cmd,QString data );

public slots:
    //void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
   bool ThreadStop;
   QSerialPort serial;
   QQueue<QByteArray> InputData;
   quint8 pdata[544]; //512 + 32 = 544
   int pdata_endindex;
   //sdw 2016/09/12  insert mutex
   QMutex rmutex;
   QMutex wmutex;

public:   
   int m_state;
   bool m_breconnect;
   QString m_serialport;
   int m_baudrate;
   QSerialPort::DataBits m_databits;
   QSerialPort::StopBits m_stopbits;
   QSerialPort::Parity m_parity;
   QSerialPort::FlowControl m_flowcontrol;

   LogDlg *plogdlg0;
   LogDlg *plogdlg1;
   int m_loglevel;
   Syslogger *log;

   int m_loopcount;


   enum ProtocolType
   {
       DLE = 0x10,
       STX = 0x02,
       ETX = 0x03,
   };

   enum TGMCU_OPCODE
   {
       CMD_Trigger = 0x41,
       CMD_TriggerREQ = 0x24,
       CMD_COMTestREQ = 0x20,  // 통신 테스트 요청
       CMD_COMTestREP = 0x21,  // 통신 테스트 응답
       CMD_SlotLaneREQ = 0x22, // Slot 및 Lan 요청
       CMD_SlotLaneREP = 0x23, // Slot 및 Lane 응답
       CMD_ACK = 0x06,
       CMD_NAK = 0x15,
       CMD_Connected = 0x00,
       CMD_Disconnected = 0x01,

       DATA_OneLane = 0x31,  //1차로
       DATA_TwoLane = 0x32,  //2차로

       DATA_Unknown = 0xE1,  // 알수없는 명령
       DATA_NoSetting = 0xE2, //설정 불가
       DATA_OK = 0x00,  // ACK - DATA
   };

};

#endif // TGMCUCOMM_H
