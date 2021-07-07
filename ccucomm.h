#ifndef CCUCOMM_H
#define CCUCOMM_H

#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>


#include <QObject>
#include <QThread>
#include <QtSerialPort/QSerialPort>
#include <QQueue>
#include <QDebug>
#include <QDateTime>
#include <QMutex>

#include "syslogger.h"
#include "logdlg.h"

class ViolationInfo
{
public:
    ViolationInfo() {}
    //복사연산자
    ViolationInfo( const ViolationInfo &other )
    {
        seq = other.seq;
        violationIndex = other.violationIndex;  //위반번호
        //violationTime = other.violationTime; //yyyyMMddHHmmss
        memcpy(violationTime,other.violationTime,sizeof(violationTime));
        violationType = other.violationType; //위반형태
        workNumber1 = other.workNumber1; //근무번호 - BCD(01~99), 차로
        workNumber2 = other.workNumber2; //근무번호 - BCD, 근무인련번호
        //workDate = other.workDate; //yyyyMMdd
        memcpy(workDate,other.workDate,sizeof(workDate));
        processNumber = other.processNumber; //처리번호
        violationCode = other.violationCode;
        //VehicleNumber = other.VehicleNumber;
        memcpy(VehicleNumber,other.VehicleNumber,sizeof(VehicleNumber));
        workType = other.workType;
        exlocalNumber = other.exlocalNumber;
        //obuNumber = other.obuNumber;
        memcpy(obuNumber,other.obuNumber,sizeof(obuNumber));

        //recvTime = other.recvTime;
        memcpy(recvTime,other.recvTime,sizeof(recvTime));

        //rawdata = other.rawdata; // opcode ~ 처리영업소번호
        memcpy(rawdata,other.rawdata,sizeof(rawdata));
    }
    //대입연산자
    ViolationInfo &operator= (const ViolationInfo &other)
    {
        seq = other.seq;
        violationIndex = other.violationIndex;  //위반번호
       // violationTime = other.violationTime; //yyyyMMddHHmmss
        memcpy(violationTime,other.violationTime,sizeof(violationTime));
        violationType = other.violationType; //위반형태
        workNumber1 = other.workNumber1; //근무번호 - BCD(01~99), 차로
        workNumber2 = other.workNumber2; //근무번호 - BCD, 근무인련번호
        //workDate = other.workDate; //yyyyMMdd
         memcpy(workDate,other.workDate,sizeof(workDate));
        processNumber = other.processNumber; //처리번호
        violationCode = other.violationCode;
       //VehicleNumber = other.VehicleNumber;
         memcpy(VehicleNumber,other.VehicleNumber,sizeof(VehicleNumber));
        workType = other.workType;
        exlocalNumber = other.exlocalNumber;
        //obuNumber = other.obuNumber;
         memcpy(obuNumber,other.obuNumber,sizeof(obuNumber));

        //recvTime = other.recvTime;
        memcpy(recvTime,other.recvTime,sizeof(recvTime));

        //rawdata = other.rawdata; // opcode ~ 처리영업소번호
        memcpy(rawdata,other.rawdata,sizeof(rawdata));
        return *this;
    }
public:
    quint8 seq;
    quint16 violationIndex;  //위반번호
    unsigned char violationTime[26]; //yyyyMMddHHmmss
    quint8 violationType; //위반형태
    quint8 workNumber1; //근무번호 - BCD(01~99), 차로
    quint8 workNumber2; //근무번호 - BCD, 근무인련번호
    unsigned char  workDate[26]; //yyyyMMdd
    quint32 processNumber; //처리번호
    quint8 violationCode;
    unsigned char  VehicleNumber[26];
    quint8 workType;
    quint16 exlocalNumber;
    unsigned char  obuNumber[26];

    //QDateTime recvTime;
    unsigned char recvTime[26]; //yyyyMMddHHmmss

#define VIOLATION_RAW_LEN   41
    //QByteArray rawdata; // opcode ~ 처리영업소번호
    unsigned char rawdata[VIOLATION_RAW_LEN];
};

class CCUSendData
{
public:
    CCUSendData() {}
    CCUSendData( const CCUSendData &other )
    {
        command = other.command;
        seq = other.seq;
        ecs = other.ecs;
        imgnumber = other.imgnumber;
        //VehicleNumber = other.VehicleNumber;
        memcpy(VehicleNumber,other.VehicleNumber,sizeof(VehicleNumber));
        confirmValue = other.confirmValue;
        //frontNumber = other.frontNumber;
        //rearNumber = other.rearNumber;
        //confirmNumber = other.confirmNumber;
        memcpy(frontNumber,other.frontNumber,sizeof(frontNumber));
        memcpy(rearNumber,other.rearNumber,sizeof(rearNumber));
        memcpy(confirmNumber,other.confirmNumber,sizeof(confirmNumber));
        senddata = other.senddata;
    }
    //대입연산자
    CCUSendData &operator= (const CCUSendData &other)
    {
        command = other.command;
        seq = other.seq;
        ecs = other.ecs;
        imgnumber = other.imgnumber;
        //VehicleNumber = other.VehicleNumber;
         memcpy(VehicleNumber,other.VehicleNumber,sizeof(VehicleNumber));
        confirmValue = other.confirmValue;
        //frontNumber = other.frontNumber;
        //rearNumber = other.rearNumber;
        //confirmNumber = other.confirmNumber;
        memcpy(frontNumber,other.frontNumber,sizeof(frontNumber));
        memcpy(rearNumber,other.rearNumber,sizeof(rearNumber));
        memcpy(confirmNumber,other.confirmNumber,sizeof(confirmNumber));
        senddata = other.senddata;

       return *this;
    }

    int command;
    quint8 seq;
    quint8 ecs;
    quint16 imgnumber;
    unsigned char VehicleNumber[26];
    quint8 confirmValue;
    unsigned char frontNumber[26];
    unsigned char rearNumber[26];
    unsigned char confirmNumber[26];

    QByteArray senddata;
};

class CCUComm : public QThread
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
    explicit CCUComm(int channel, int loglevel = LOG_INFO,QThread *parent = nullptr);
    ~CCUComm();
    void stop();
    void run();
    int OpenSerial();
    int OpenSerial(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits
                   ,QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    int OpenSerial(QString portName, int baudrate, QSerialPort::DataBits databits
                   ,QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    void CloseSerial();
    void Parsing(char *rdata, int rdatalen);
    void ProcessCommand(quint8 *cmddata,int cmdlen);
    QByteArray Minus0x10(QByteArray rxdata);
    void SerialDeviceSetting();
    void SerialDeviceSetting(bool bReconnect, QString portName, int baudrate, QSerialPort::DataBits databits
                                 , QSerialPort::StopBits stopbits, QSerialPort::Parity parity, QSerialPort::FlowControl flowcontrol);
    void writeData(QByteArray data);

    QByteArray MakeSendPacket(quint8 Opcode, quint8 Seq, QByteArray Data = NULL);
    quint8 MakeBCC(QByteArray data);
    bool CheckBCC(QByteArray data);
    QByteArray Uint16toArr(quint16 number);
    quint32 ArrtoUint(QByteArray bdata);
    void ACK_Send(quint8 seq);
    void NAK_Send(quint8 seq);
    void StatusREP_Send(quint8 seq, quint8 ecs, quint16 imgnumber);
    void VIOLATION_REQ_Send(quint8 seq, quint16 imgnumber);
    void Vehicle_Notification(quint8 seq, quint16 imgnumber, QString vehiclenumber);
    void Vehicle_Notification_NEW(quint8 seq, quint16 imgnumber
                                  , QString frontNumber, QString rearNumber, QString confirmNumber
                                  ,quint8 confirmVaule);

    QByteArray GetVehicleNumberByte(QString VehicleNumber);
    QString GetVehicleNumber_byCode(QByteArray bNumber);
    quint8 GetRegionCode(QString sNum1,bool bCommercial);
    QString GetRegion_byCode(quint8 bNumber);
    quint8 GetCarClass(QString sNum3);
    QString GetCarClass_byCode(quint8 bNumber);

signals:
    void sigReceiveHandler(int channel, quint8 Cmd,QString data,QByteArray bdata );
public slots:
    //void readData();
    //void handleError(QSerialPort::SerialPortError error);

private:
   bool ThreadStop;
   //QSerialPort serial;
   int serial;
   struct termios oldtio, newtio;
   struct termios oldkey, newkey;

   QList<QByteArray> InputData;
#define  CCU_MAX_RECVBUFF 1024
   quint8 pdata[CCU_MAX_RECVBUFF];
   int pdata_endindex;
   //sdw 2016/09/12  insert mutex
   QMutex rmutex;
   QMutex wmutex;

   QList <CCUSendData> OutputData;
   QDateTime m_lastoutputtime;
   QDateTime m_viodelaytime;
   QList <CCUSendData> VioREQData;


public:
   int m_state;
   int m_channel;
   quint8 m_sequenceindex;
   bool m_breconnect;
   QString m_serialport;
   int m_baudrate;
   QSerialPort::DataBits m_databits;
   QSerialPort::StopBits m_stopbits;
   QSerialPort::Parity m_parity;
   QSerialPort::FlowControl m_flowcontrol;

   LogDlg *plogdlg;
   int m_loglevel;
   Syslogger *log;

   bool m_bsyncREQsend;
   int m_loopcount;
   int m_bccerrorcount;
#define MAXBCCERRORCOUNT  5

   enum ProtocolType
   {
       DLE = 0x10,
       STX = 0x02,
       ETX = 0x03,
       BCCLEN = 0x01
   };

   enum CCU_OPCODE
   {
       CMD_StatusREQ = 0x11,  //상태요구
       CMD_StatusREP = 0x21,  //상태응답

       CMD_WORKSTART = 0x31,  //근무개시
       CMD_WORKEND = 0x32,  //근무종료

       CMD_VIOLATION_REQ = 0x41,  // 위반확인요구
       CMD_VIOLATION_REP = 0x42,  // 위반확인응답
       CMD_VIOLATION_REP_NEW = 0x43, //신위반확인응답(현재는 이 명령만 사용 )
       CMD_Vehicle_Notification = 0x44, //차량번호 통보
       CMD_Vehicle_Notification_NEW = 0x47, //차량번호통보 - 전후면용
       CMD_CONFIRM_INFO = 0x45,  //번호판 확정 명령

       CMD_VIOLATION_SYNC = 0x25,    //위반번호 Sync
       CMD_SPEC_ENTER_BEGIN = 0x51,  //입구특별발행 개시
       CMD_SPEC_ENTER_END = 0x52,    //입구특별발행 종료

       CMD_ACK = 0x06,
       CMD_NAK = 0x15,

       CMD_Connected = 0x00,
       CMD_Disconnected = 0x01

   };

};

#endif // CCUCOMM_H
