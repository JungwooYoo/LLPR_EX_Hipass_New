#ifndef MAINTHREAD_H
#define MAINTHREAD_H

#include <QThread>
#include <QDebug>
#include <QList>
#include <QImage>
#include <QPainter>
#include <QTextCodec>

//#include "centerthread.h"
#include "dbmng.h"
#include "logdlg.h"
#include "syslogger.h"
#include "dataclass.h"
#include "ccucomm.h"

class mainthread : public QThread
{
    Q_OBJECT
public:
    explicit mainthread(CCUComm *pccu0, CCUComm *pccu1 ,int loglevel = LOG_INFO,QThread *parent = nullptr);
    ~mainthread();
    void SetLogLevel(int loglevel);
    bool start();
    void run();
    void stop();
    void center_check();
    void createjpegimage(int channel, int index);
    void VioRepCheck(int channel);
    void FTPSend(int channel,int vioresultIndex, ViolationInfo violationinf, bool bmissmatch = false);
    QByteArray MakeFTPFile(int channel, int vioresultIndex, ViolationInfo violationinf, bool bmissmatch, QString *filename1, QString *filename2, quint32 *processNum);
    QByteArray MakeViolationData(ViolationInfo violationinf);
    QByteArray Uint32toArr_Little(quint32 number);
    void ConfirmInfo(int channel, quint32 ProcessNumber, int confirmCam);
    void InsertFTPData(quint8 seq, quint16 imgNum, quint8 front_rear, QString vehicleNumber, QString fileName1, QString fileName2, QByteArray ftpdata, clsHipassCarData *cardata);
    void VehicleNotification_NEW();
    QByteArray OBUMatch(int channel, int vioresultIndex, ViolationInfo violationinf);


signals:
    void createcenter(int index);
    void connectcenter(int index);

public slots:

public:
    bool brun;
    CCUComm *pccucomm0;
    CCUComm *pccucomm1;
    dbmng *pdatabase;
    LogDlg *plogdlg0;
    LogDlg *plogdlg1;
    Syslogger *log;
    int m_loglevel;

    QList<VehcileIndex> procindex;
    QList<int> ViolationImageList[2];
    QList<clsFTPDATA> FTPDataList[2];
    #define MAX_CONFIRMDATA   3
    QTextCodec * codec;
    QTextEncoder *encoderw;

    int m_isignalflag;
    enum SIGNALCHECK
    {
        SIG_OnTimer = 0x0001,
        SIG_LICRcv = 0x0002,
        SIG_CCURcv = 0x0004,
        SIG_TGMCURcv = 0x0008
    };

};

#endif // MAINTHREAD_H
