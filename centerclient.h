#ifndef CENTERCLIENT_H
#define CENTERCLIENT_H

#include <QtCore>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QTextStream>
#include <QDebug>
#include <QTextEncoder>
#include <QMutex>
#include <QMetaEnum>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFtp>


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
#include "syslogger.h"
#include "logdlg.h"
#include "ipsocket.h"
//#include "ftpclient.h"
#include "dataclass.h"

using namespace std;

class CenterClient : public QThread, public ipsocket
{
    Q_OBJECT
private:
    class SendFileInfo
    {
    public:
        SendFileInfo()
        {
            filename = "";
            filepath = "";
        }

        bool SaveFile(QString path, QString fname, QByteArray filedata);
        bool ParseFilepath(QString _filepath); //센터로 보낼 데이터인지 파일명을 체크해서 리턴함.

    public:
        QString filename;
        QString filepath;
    };

    class SendFileInfoList
    {
    public:
        bool AddFile(SendFileInfo data);
        SendFileInfo GetFirstFile();
        void RemoveFirstFile(SendFileInfo data);
        void ClearAll();
        int Count();
    private:
        QList<SendFileInfo> fileList;
        const int MAX_FILE = 10000;
        QMutex mutex;


    };


public:
    explicit CenterClient(int id,QString centerName,int loglevel = LOG_INFO,QThread *parent = nullptr);
    ~CenterClient();
    void SetLogLevel(int loglevel);
    void run();
    void stop();
    void receivedatacheck();
    bool Connect(QString ip, QString port, QString centerName,int mkaminterval);
    void ConnectAsync(QString ip, QString port, QString centerName,int mkaminterval);
    bool Disconnect();
    int Parsing();
    int ParsePacket(unsigned char* data, int datalen);

    bool SendData(QString strdata);
    bool InitConnection(uint laneID);
    bool StatusREP(uint laneID);
    bool LaneVersion(uint version);
    bool VehicleTypeReqREP(QStringList CarType);
    bool VehicleTypeChangeREP(QStringList CarType);
    bool CurrentDateREP();
    bool CCUCommStatus(bool bstatus,uint laneID);
    bool CameraPortStatus(bool bstatus, uint laneID);
    bool LightStatus(bool bstatus, uint laneID);
    bool BoothStatus(bool bstatus, uint laneID);
    void Status_Send();

    //원격관제통신
    int RemoteParsing();
    bool Analysis(QByteArray orgdata);
    QJsonObject MakeCommon(QString datatype,QString msgtype);
    bool jSendData(QJsonObject jdata);
    bool WorkStark_Send();
    bool LaneStatus_Send();
    bool BosuData_Send();
    bool FwUpdateResult();
    //처리차량데이터하이패스
    bool AddCarNoData(clsHipassCarData *cardata);
    bool CarNoDataHipass_Send(clsHipassCarData *cardata);
    bool CommandRESP(QString datatype, QString msg, QString data);
    bool FTPDownload(QString sip, QString sport, QString sid, QString spassword,QString filePath, QString fileName);
    bool OBUFTPDownload(QString sip, QString sport, QString sid, QString spassword,QString filePath, QString fileName);
    bool OBUUpdate(QString fileName);
    //FTP download
    void connectToFtp(QString sIP, QString sPort, QString sID, QString sPW, QString sPath);
    void disconnectToFtp();
    bool downloadFile(QString fileName);

    //FTP 파일저장
    bool SendVehicleInfo( QString fename, QByteArray ftpData);
    bool FtpSendVehicleInfo(QString fename, QByteArray ftpData);
    void ScanSendDataFiles();
    void DeleteFile(QString filepath);

signals:
    void Connected();
    void Disconnected();
    void CommandEvent(int id,QString cmd, QString data = NULL);

public slots:
    void ftpCommandFinished(int id,bool error);
    void cancelDownload();
public:
    bool brun;
    QString m_ip;
    QString m_centerName;

    int connectioncount;
    int m_kaminterval;

    int setconnectioncount;
    int centerid;

    QMutex wmutex;
    QMutex rmutex;
    QString m_transstate;
    bool m_bconnflag;

    Syslogger *log;
    int m_loglevel;
    LogDlg *plogdlg0;
    LogDlg *plogdlg1;

    QTextCodec *codec;
    QTextEncoder *encoderw;

    SendFileInfoList sendFileList;
    //QString FTP_SEND_PATH;
    int fileNameSelect;
    int protocol_type;

    //remote protocol
    unsigned char* m_prdata;
    int m_prdatalen;
    int m_workstartRetry;
    int m_workstartflag;
    QFtp *m_pftp;
    QFile *m_pftpfile;
#define MAX_FTPDOWNLOAD_COUNT  1000
    int m_iFiledownloadCnt;
    bool m_bDownload;
    //bool m_bOBU_Update;
    QString m_OBUFileName;
    enum ProtocolValue
    {
        PROTOCOL_FILESIZE = 5
    };
#define MAX_REMOTEDATA  50
    QList<clsHipassCarData*> remoteCarDatalist;

#define REMOTEDATA_BUF 3072
    char reuckr_buf[REMOTEDATA_BUF];
    char rutf8_buf[REMOTEDATA_BUF];
    char teuckr_buf[REMOTEDATA_BUF];
    char tutf8_buf[REMOTEDATA_BUF];

};

#endif // CENTERCLIENT_H
