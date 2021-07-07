#ifndef RECOGTHREAD_H
#define RECOGTHREAD_H
#include <QObject>
#include <QThread>
#include <QQueue>
//#include "logger.h"
#include <QImage>
#include <QColor>
#include <QDebug>
#include <QTextCodec>
#include <QDateTime>
#include <QDebug>
#include <QApplication>
//ALPR
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // exit()
#include <fstream>
#include <sstream>
#include <iostream>
//

#include "lib/ilpr_1.5.2.0u_2/iplpr.h"
#include "syslogger.h"
#include "lib/alpr_ipu_v3.6.5_lt/ALPR_api.h"
#include "dataclass.h"


#include "logdlg.h"

//#include "lprdemo.h"

//extern "C" {
//#include  "lprdemo.h"
//}
typedef unsigned short WORD;
typedef unsigned char byte;
typedef unsigned char PIX_TYPE;

class recogthread :public QThread
{
    Q_OBJECT

public:
    enum CameraView
    {
        Center = 0x00,
        CamLeft_CarRight = 0x01,
        CamRight_CarLeft = 0x02
    };
    enum RecogMode
    {
        ALPR_MODE,
        iLPR_MODE,
        ALPR_iLPR_MODE,
        iLPR_ALPR_MODE,
        DUAL_ALPR_iLPR_MODE,
        DUAL_iLPR_ALPR_MODE
    };
    enum RecogLicenseStatus
    {
        ALL_LICENSE_OK = 0x00,
        ALPR_LICENSE_FAIL = 0x01,
        iLPR_LICENSE_FAIL = 0x02,
    };

//    struct RecogImage{
//        quint32 index;
//        //unsigned char *img;//Image img;
//        Image img;
////        QByteArray img;
////        int width;
////        int height;
//    };
    struct Recogresult
    {
        int channel;
        int plateX;
        int plateY;
        int plateWidth;
        int plateHeight;
        int plateType;
        int plateIntensity;
        char szPlateNumber[256];
        double dblRecogTime;
        int RecogShift;
        int fileIndex;
        int iSaveLog;
        char szLogFile[256];
    };

public:
    explicit recogthread(int loglevel = LOG_INFO,QThread *parent = 0);
    ~recogthread();
    void SetLogLevel(int loglevel);
    void init();
    void run();
    void stop();
    void initRecog(int width,int height);
    void recogsetting();
    //void RecogImageInsert(RecogImage recogimge);
    int doRecognition(int channel,int index,Recogresult *recogres);
    int Recog_ALPR(int channel,int index,Recogresult *recogres,int img_sx = -1, int img_sy = -1,int img_width = -1, int img_height = -1);
//    void Recog_ALPR_Result(int rtn, Recogresult recogres);
    int Recog_iLPR(int channel,int index,Recogresult *recogres, int img_sx = -1,int img_sy = -1, int img_ex = -1, int img_ey = -1);
    int Recog_ALPR_iLPR(int channel,int index,Recogresult *recogres);
    int Recog_iLPR_ALPR(int channel,int index,Recogresult *recogres);
//    bool ConvertBayerRawIntoGray(int channel,int index, int width, int height
//                                 , cv::Mat &img_8bit, cv::Mat &img_12bit
//                                 , int shift, int cameraType);

    void RGBToGray(WORD *img, int rawW, int rawH);
    void Make8BitImage(WORD * p12Bit, int rawW, int rawH, int shiftIndex, PIX_TYPE * pData);
    QString MatchLPDATA(QString strkey);
    bool GetLicenseStatus();
    int CheckVehicleNumber(std::vector<LPResult> resultList);
    QString CheckYeong(Recogresult *recogres);

public slots:

signals:
    void RecogEndSignal(int channel,int index);
    //void RecogALPRSignal(int index);

public:
    bool brun;
    int recog_iLPRType[2];
    int image_width[2];
    int image_height[2];
    int recog_plate_sx[2];
    int recog_plate_sy[2];
    int recog_plate_ex[2];
    int recog_plate_ey[2];
    int recog_iCamView[2];
    int recog_iShftCount[2];
    int recog_arShift[2][8];
    RecogMode recog_mode[2];
    IPLPR *lpr;
    //sdw //2016/09/27
    ALPR_api    *alpr;
    bool balpr_result;
    int  alprreturn;
    Recogresult m_recogres;

    //sdw //2016/10/11
    QTextCodec * codec;

    //QQueue<RecogImage> Recogimg;    
    QString rawdir;
    QList<VehcileIndex> procindex;
    int irecoglicense;

    Syslogger *log;
    int m_loglevel;
    LogDlg *plogdlg0;
    LogDlg *plogdlg1;




};

#endif // RECOGTHREAD_H
