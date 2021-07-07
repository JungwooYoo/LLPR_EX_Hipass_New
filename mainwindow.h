#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


#include "cameraconfigdlg.h"
#include "apsgdlg.h"
#include "configdlg.h"
#include "centerdlg.h"
#include "controlboarddlg.h"
#include "databasedlg.h"
#include "logdlg.h"
#include "recognitiondlg.h"

#include "config.h"
#include "spinview.h"
#include "autoiris.h"
#include "ccucomm.h"
#include "liccomm.h"
#include "tgmcucomm.h"
#include "mainthread.h"
#include "recogthread.h"
#include "dbmng.h"
#include "centerclient.h"
#include "syslogger.h"
#include "commonvalues.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void SetLogLevel(int loglevel);
    void initaction();
    void initwindows();
    void init();
    void init_config();
    void init_vehiclemem();
    void free_vehiclemem();
    void init_camera();
    void init_controlboard();
    void init_recog();
    void init_mainthr();
    void init_autoiris();
    void init_database();
    void init_remotevalue();
    //void init_etc();
    void applyconfig2common();
    void applycommon2config();
    void checkcenterstatus();
    void loglevelsetting(int loglevel);
    void keyPressEvent(QKeyEvent *event);
    //void SerialCheck();
    quint32 ArrtoUint(QByteArray bdata);
    bool SerialThreadCheck();

signals:
    void BoardDlgTextUpdate(int channel,int board,quint8 Cmd,QString data);
    void camera_restart(int channel);

public slots:
    void cameraConfigview0();
    void apsgdlgview0();
    void controlboarddlgview0();
    void recognitiondlgview0();
    void logdlgview0();
    void cameraConfigview1();
    void apsgdlgview1();
    void controlboarddlgview1();
    void recognitiondlgview1();
    void logdlgview1();
    void centerdlgview();
    void databasdlgeview();
    void configurationdlgview();

    void sigLICReceiveHandler(int channel,quint8 Cmd,QString data );
    void sigCCUReceiveHandler(int channel,quint8 Cmd,QString data,QByteArray bdata );
    void sigTGMCUReceiveHandler(quint8 Cmd,QString data );
    void sigCameraStart(int channel);
    void capture(int channel, int index, QImage *img);
    void recogcapture(int channel, int index);
    void RecogEndSignal(int channel,int index);
    void SetLICStrobe(int channel,bool lightonoff);
    void LICTrigger(int channel);

    void recogdlgsaveslot(int channel);

    void createcenter(int index);
    void connectcenter(int index);
    void CenterCommandEvent(int id, QString cmd, QString data);
    void OBUUpdateResult(int id,QString sControlNo,bool bupdate, QString sFilename);

    void closeEvent(QCloseEvent *);
    void onTimer();
    void CurrentValueStatus();
    void DeviceStatus();
    void loginsetting(bool loginstate, int logintime = 600);

private slots:
    void on_btnLogin_clicked();
    void on_btnLogLevelSet_clicked();
    void on_chkDisplay0_clicked();
    void on_chkDisplay1_clicked();

    void on_btnCCUReconnection_clicked();

private:
#define  Program_Name   "LLPR_EX_HIPASS"
#define  Program_Version  "1.2.6.7"
#define  Program_Date   "2020/10/25"
    Ui::MainWindow *ui;
    //dialog
    CameraConfigDlg *pcameraconfigdlg0;
    CameraConfigDlg *pcameraconfigdlg1;
    APSGDlg *papsgdlg0;
    APSGDlg *papsgdlg1;
    ControlBoardDlg *pcontrolboarddlg0;
    ControlBoardDlg *pcontrolboarddlg1;
    RecognitionDlg *precognitiondlg0;
    RecognitionDlg *precognitiondlg1;
    LogDlg *plogdlg0;
    LogDlg *plogdlg1;

    configdlg *pconfigdlg;
    DatabaseDlg *pdatabasedlg;
    CenterDlg *pcenterdlg;

    //class
    config *pcfg;
    spinview *pcamera0;
    spinview *pcamera1;
    LICComm *pliccomm0;
    LICComm *pliccomm1;
    CCUComm *pccucomm0;
    CCUComm *pccucomm1;
    TGMCUComm *ptgmcucomm;
    recogthread *precogthr;
    mainthread *pmainthr;
    dbmng *pdatabase;
    autoiris *pautoiris0;
    autoiris *pautoiris1;
    Syslogger *plog;

    QTimer *qTimer;
    bool m_blogin;
    int m_logintime;
    int m_loglevel;
    //camera restart count
    int m_restartcount[2] ;
    int m_mseccount;

    QDateTime bootingTime;
    bool bclose;

    QDateTime lastTGTime[2];



};


//1ch 558x760
//2ch 1111x760
#endif // MAINWINDOW_H
