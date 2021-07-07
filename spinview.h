#ifndef SPINVIEW_H
#define SPINVIEW_H

#include <QObject>
#include <QThread>
#include <QMutex>


#include <Spinnaker.h>
#include <SpinGenApi/SpinnakerGenApi.h>
#include <iostream>
#include <sstream>


#include "logdlg.h"
#include "syslogger.h"

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

//#define _DEBUG

class spinview : public QThread
{
    Q_OBJECT
public:
    enum CameraState
        {
            NORMAL = 0x0000,
            INITFAIL = 0x0001,
            STARTFAIL = 0x0002,
            CONFIGSETFAIL = 0x0004,
            CAPTUREFAIL = 0x0008,
            STOP = 0x0010,
            HOLDING = 0x0020
        };
public:
    explicit spinview(int channel,int loglevel = LOG_INFO, QThread *parent = nullptr);
    ~spinview();
    void SetLogLevel(int loglevel);
    bool init();
    bool start(QString strCamIP ,uint serialNumber = 0);
    bool start();
    void run();
    void stop();
    //void close();
    bool StartCamera(QString strCamIP = "" ,uint serialNumber = 0);
    void StopCamera();
    QString ConvertIPAddress(int64_t ipaddr);
    bool GetBright(unsigned int *bright);

public:
    bool SetCameraSetting();
    bool SetCameraAcquisitionMode(QString strmode = "Continuous");
    bool SetCameraAutoExposure(QString strmode = "Off");
    bool SetCameraAutoGain(QString strmode = "Off");
    bool SetCameraWhiteBalanceMode(QString strmode = "Off");
    bool SetCameraWhiteBalanceRed(float fvalue);
    bool GetCameraWhiteBalanceRed(float *fvalue);
    bool SetCameraWhiteBalanceBlue(float fvalue);
    bool GetCameraWhiteBalanceBlue(float *fvalue);
    bool SetPixelFormatRaw(QString strmode = "BayerRG16");
    bool SetImageSize(int width, int height,int offsetX, int offsetY);
    bool GetImageSize(int *width, int *height, int *offgetX, int *offgetY);
    bool SetCameraPacketSize_Delay(uint packetsize, uint packetdelay);
    bool SetCameraShutter(float fvalue);
    bool GetCameraShutter(float *fvalue);
    bool SetCameraGain(float fvalue);
    bool GetCameraGain(float *fvalue);
    bool SetCameraFrameRate(float fvalue, bool bAuto, bool bOnOff);
    bool SetCameraStrobe(bool benable,bool bpolarity, float fdelay, float fduration);
    bool GetCameraStrobe(bool *benable,bool *bpolarity, float *fdelay, float *fduration);



signals:
    //void display(int channel,int index,QImage *img);
    void capture(int channel,int index,QImage *img);
    void recogcapture(int channel,int index);
    void sigStart(int channel);
    void sigStop(int channel);
    //void camera_restart(int channel);

public slots:
    void restart(int channel);

public :
    bool brun;
//    cameradlg* pcamdlg;
//    APSGdlg* papsgdlg;
//    autoiris* pautoiris;

    SystemPtr system;
    CameraList camList;
    CameraPtr pcamera;
    uint cameraSerialNumber;
    QString IPAddress;
    uint SerialNumber;
    bool bdisplay;

    int image_width;
    int image_height;

    QList<int> procindex;
    Syslogger *log;
    int m_loglevel;
    LogDlg *plogdlg;
    int m_state;
    int m_channel;

    //image br calc
    QMutex brightmux;
    unsigned char m_brightrawimg[5*1024*1024];
    int m_brightrawimglen;



};

#endif // SPINVIEW_H
