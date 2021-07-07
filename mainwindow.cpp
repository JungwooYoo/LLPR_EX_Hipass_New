#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->btnCCUReconnection->setVisible(false);

    pcameraconfigdlg0 = NULL;
    pcameraconfigdlg1 = NULL;
    papsgdlg0 = NULL;
    papsgdlg1 = NULL;
    pcontrolboarddlg0 = NULL;
    pcontrolboarddlg1 = NULL;
    precognitiondlg0 = NULL;
    precognitiondlg1 = NULL;
    plogdlg0 = NULL;
    plogdlg1 = NULL;

    pconfigdlg = NULL;
    pdatabasedlg = NULL;
    pcenterdlg = NULL;

    //class
    pcfg = NULL;
    pcamera0 = NULL;
    pcamera1 = NULL;
    pliccomm0 = NULL;
    pliccomm1 = NULL;
    pccucomm0 = NULL;
    pccucomm1 = NULL;
    ptgmcucomm = NULL;
    precogthr = NULL;
    pmainthr = NULL;
    pdatabase = NULL;
    pautoiris0 = NULL;
    pautoiris1 = NULL;
    plog = NULL;

    bootingTime = QDateTime::currentDateTime();
    bclose = false;

    lastTGTime[0] = QDateTime::currentDateTime();
    lastTGTime[1] = QDateTime::currentDateTime();

    init();


    loginsetting(true);
}

MainWindow::~MainWindow()
{
    //dialog
    if(pcameraconfigdlg0 != NULL) delete pcameraconfigdlg0;
    if(pcameraconfigdlg1 != NULL) delete pcameraconfigdlg1;
    if(precognitiondlg0 != NULL) delete precognitiondlg0;
    if(precognitiondlg1 != NULL) delete precognitiondlg1;
    if(plogdlg0 != NULL) delete plogdlg0;
    if(plogdlg1 != NULL) delete plogdlg1;
    if(pconfigdlg != NULL) delete pconfigdlg;
    if(pcenterdlg != NULL) delete pcenterdlg;
    if(pdatabasedlg != NULL ) delete pdatabasedlg;

    if(pcfg != NULL) delete pcfg;
    if(pautoiris0 != NULL) delete pautoiris0;
    if(pautoiris1 != NULL) delete pautoiris1;
    if(pcameraconfigdlg0 != NULL) delete pcameraconfigdlg0;
    if(pcameraconfigdlg1 != NULL) delete pcameraconfigdlg1;

    if(precogthr != NULL) delete precogthr;
    if(pcontrolboarddlg0 != NULL) delete pcontrolboarddlg0;
    if(pcontrolboarddlg1 != NULL) delete pcontrolboarddlg1;
    if(pmainthr != NULL) delete pmainthr;
    if(pdatabase != NULL) delete pdatabase;

    delete ui;
}


void MainWindow::initaction()
{
    connect(ui->actionCameraConfig0,SIGNAL(triggered()),this,SLOT(cameraConfigview0()));
    connect(ui->actionAPSG0,SIGNAL(triggered()),this,SLOT(apsgdlgview0()));
    connect(ui->actionControlBoard0,SIGNAL(triggered()),this,SLOT(controlboarddlgview0()));
    connect(ui->actionRecognition0,SIGNAL(triggered()),this,SLOT(recognitiondlgview0()));
    connect(ui->actionLog0,SIGNAL(triggered()),this,SLOT(logdlgview0()));

    connect(ui->actionCameraConfig1,SIGNAL(triggered()),this,SLOT(cameraConfigview1()));
    connect(ui->actionAPSG1,SIGNAL(triggered()),this,SLOT(apsgdlgview1()));
    connect(ui->actionControlBoard1,SIGNAL(triggered()),this,SLOT(controlboarddlgview1()));
    connect(ui->actionRecognition1,SIGNAL(triggered()),this,SLOT(recognitiondlgview1()));
    connect(ui->actionLog1,SIGNAL(triggered()),this,SLOT(logdlgview1()));

    connect(ui->actionCenter,SIGNAL(triggered()),this,SLOT(centerdlgview()));
    connect(ui->actionDB,SIGNAL(triggered()),this,SLOT(databasdlgeview()));
    connect(ui->actionConfiguration,SIGNAL(triggered()),this,SLOT(configurationdlgview()));
}

void MainWindow::initwindows()
{
    QString title = QString("%1 v%2 (date: %3)")
            .arg(Program_Name).arg(Program_Version).arg(Program_Date);

    this->setWindowTitle(title);
    ui->leLoginPW->setEchoMode(QLineEdit::Password);
    ui->lblCapImg0->setScaledContents(true);
    ui->lblCapImg1->setScaledContents(true);
    ui->lblPlateImg0->setScaledContents(true);
    ui->lblPlateImg1->setScaledContents(true);

    //1ch 558x760  //2ch 1111x760
    //windows fixed
    if( commonvalues::cameraChannel > 1 )
        this->setFixedSize(this->geometry().width(),this->geometry().height());
    else
    {
        this->setGeometry(this->geometry().x(),this->geometry().y()
                            ,558,this->geometry().height());
        this->setFixedSize(this->geometry().width(),this->geometry().height());

        ui->menuChannel1->setEnabled(false);
    }
}

void MainWindow::init()
{
    pcfg = new config();
    init_config();
    initwindows();
    init_remotevalue();

    plog = new Syslogger(this,"mainwindow",true,commonvalues::loglevel);
    m_loglevel = commonvalues::loglevel;

    QString title = QString("%1 v%2 (date: %3)")
            .arg(Program_Name).arg(Program_Version).arg(Program_Date);
    QString logstr = QString("Program Start(%1)").arg(title);
    plog->write(logstr,LOG_ERR); qDebug() << logstr;

    ui->cbLogLevel->setCurrentIndex(m_loglevel);

    for(int index = 0; index < commonvalues::cameraChannel; index++)
    {
        if(index == 0)
        {
            pcamera0 = new spinview(index,m_loglevel);
            pliccomm0 = new LICComm(index,m_loglevel);
            pccucomm0 = new CCUComm(index,m_loglevel);
            pautoiris0 = new autoiris(index,pcamera0,m_loglevel);
        }
        else if( index == 1)
        {
            pcamera1 = new spinview(index,m_loglevel);
            pliccomm1 = new LICComm(index,m_loglevel);
            pccucomm1 = new CCUComm(index,m_loglevel);
            pautoiris1 = new autoiris(index,pcamera1,m_loglevel);
        }

    }
    ptgmcucomm = new TGMCUComm(m_loglevel);
    precogthr = new recogthread(m_loglevel);
    pmainthr = new mainthread(pccucomm0,pccucomm1,m_loglevel);
    pdatabase = new dbmng(m_loglevel);

    //create dialog
    for(int index = 0; index < commonvalues::cameraChannel; index++)
    {
        if(index == 0)
        {
            plogdlg0 = new LogDlg(index);
            pliccomm0->plogdlg = plogdlg0;
            pccucomm0->plogdlg = plogdlg0;
            ptgmcucomm->plogdlg0 = plogdlg0;
            pcamera0->plogdlg = plogdlg0;
            precogthr->plogdlg0 = plogdlg0;
            pmainthr->plogdlg0 = plogdlg0;
            pdatabase->plogdlg0 = plogdlg0;

            pcameraconfigdlg0 = new CameraConfigDlg(index,pcamera0,pcfg);
            papsgdlg0 = new APSGDlg(index,pautoiris0,pcfg);
            pcontrolboarddlg0 = new ControlBoardDlg(index,pliccomm0,pccucomm0,ptgmcucomm,pcfg);
            connect(pcontrolboarddlg0,SIGNAL(SoftTrigger(int)),this,SLOT(LICTrigger(int)));
            connect(this,SIGNAL(BoardDlgTextUpdate(int,int,quint8,QString)),pcontrolboarddlg0,SLOT(TextUpdate(int,int,quint8,QString)));
            precognitiondlg0 = new RecognitionDlg(index, precogthr, pcfg);
            //connect(precognitiondlg0,SIGNAL(saveevent(int)),this,SLOT(recogdlgsaveslot(int ))); //channel info
        }
        else if(index == 1)
        {
            plogdlg1 = new LogDlg(index);
            pliccomm1->plogdlg = plogdlg1;
            pccucomm1->plogdlg = plogdlg1;
            ptgmcucomm->plogdlg1 = plogdlg1;
            pcamera1->plogdlg = plogdlg1;
            precogthr->plogdlg1 = plogdlg1;
            pmainthr->plogdlg1 = plogdlg1;
            pdatabase->plogdlg1 = plogdlg1;

            pcameraconfigdlg1 = new CameraConfigDlg(index,pcamera1,pcfg);
            papsgdlg1 = new APSGDlg(index,pautoiris1,pcfg);
            pcontrolboarddlg1 = new ControlBoardDlg(index,pliccomm1,pccucomm1,ptgmcucomm,pcfg);
            connect(pcontrolboarddlg1,SIGNAL(SoftTrigger(int)),this,SLOT(LICTrigger(int)));
            connect(this,SIGNAL(BoardDlgTextUpdate(int,int,quint8,QString)),pcontrolboarddlg1,SLOT(TextUpdate(int,int,quint8,QString)));
            precognitiondlg1 = new RecognitionDlg(index, precogthr, pcfg);
            //connect(precognitiondlg1,SIGNAL(saveevent(int)),this,SLOT(recogdlgsaveslot(int))); //channel info
        }
    }
    pconfigdlg = new configdlg(pcfg);
    pdatabasedlg = new DatabaseDlg(pdatabase,pcfg);
    pcenterdlg = new CenterDlg(pmainthr,pcfg);

    //action signal connection
    initaction();
    //thread init
    init_database();
    init_camera();
    init_vehiclemem();
    //precogdlg->init after init_camera
    precognitiondlg0->init();
    if(commonvalues::cameraChannel > 1 )precognitiondlg1->init();
    //init_recog depend init_camera
    init_recog();    
    init_mainthr();
    //finally spc start
    //trigger after spc start(init_spc)
    init_controlboard();
    init_autoiris();

    //QTimer init
    qTimer = new QTimer();
    connect(qTimer,SIGNAL(timeout()),this,SLOT(onTimer()));
    qTimer->start(1000);
    m_mseccount=0;
}

void MainWindow::init_config()
{
    //check config file exists
    if( !pcfg->check())
    {
        //create config file emptied
        pcfg->create();

        //load config file
        pcfg->load();
        //init config value
        applycommon2config();

        pcfg->save();
    }
    else
    {
        //load config file
        pcfg->load();
        //apply commonvalues
        applyconfig2common();

    }
}

void MainWindow::init_vehiclemem()
{   
    for(int ch = 0; ch < commonvalues::cameraChannel; ch++)
    {
        commonvalues::vehicledatalist[ch] = (VehicleData *)malloc(sizeof(VehicleData)*MAX_VEHICLE);
        memset(commonvalues::vehicledatalist[ch],0,sizeof(VehicleData)*MAX_VEHICLE);

        int width = commonvalues::cameraSys[ch].cam_image_width;
        int height = commonvalues::cameraSys[ch].cam_image_height;
        int rawimagelen = sizeof(unsigned char) * width * height * 2 + 10;
        int rgbimagelen = sizeof(unsigned char) * width * height * 3 + 10;
        int rgbplateimagelen = sizeof(unsigned char) * 500 * 500 * 3;
        int jpegimagelen = sizeof(unsigned char) * 512 * 1024; //500Kbyte

        for(int i=0; i < MAX_VEHICLE; i ++)
        {
            commonvalues::vehicledatalist[ch][i].img = (unsigned char *)malloc(rgbimagelen);
            commonvalues::vehicledatalist[ch][i].plateImage = (unsigned char *)malloc(rgbplateimagelen);
            commonvalues::vehicledatalist[ch][i].saveimg = (unsigned char *)malloc(jpegimagelen);
            commonvalues::vehicledatalist[ch][i].saveplateimg = (unsigned char *)malloc(jpegimagelen);
            commonvalues::vehicledatalist[ch][i].rawImage = (unsigned char *)malloc(rawimagelen);
        }

    }

    //FTPDATA
    commonvalues::confirmdatalist = (ConfirmData *)malloc(sizeof(ConfirmData) * MAX_CONFIRM_VEHICLE);
    memset(commonvalues::confirmdatalist,0,sizeof(ConfirmData) * MAX_CONFIRM_VEHICLE);

    int jpegimglen = sizeof(unsigned char) * 512 * 1024; //500Kbyte
    for( int i=0; i < MAX_CONFIRM_VEHICLE; i++)
    {
        commonvalues::confirmdatalist[i].fFTPData = (unsigned char *)malloc(jpegimglen);
        commonvalues::confirmdatalist[i].rFTPData = (unsigned char *)malloc(jpegimglen);
    }


}

void MainWindow::free_vehiclemem()
{
    for(int ch = 0; ch < commonvalues::cameraChannel; ch++)
    {
        for(int i=0; i < MAX_VEHICLE; i ++)
        {
            free(commonvalues::vehicledatalist[ch][i].img );
            free(commonvalues::vehicledatalist[ch][i].plateImage );
            free(commonvalues::vehicledatalist[ch][i].saveimg );
            free(commonvalues::vehicledatalist[ch][i].saveplateimg );
            free(commonvalues::vehicledatalist[ch][i].rawImage );
        }

    }

    //FTPDATA
    for( int i=0; i < MAX_CONFIRM_VEHICLE; i++)
    {
        free(commonvalues::confirmdatalist[i].fFTPData);
        free(commonvalues::confirmdatalist[i].rFTPData);
    }
}

void MainWindow::init_camera()
{
    for(int ch=0; ch < commonvalues::cameraChannel; ch++)
    {
        if( ch == 0)
        {
            pcamera0->start(commonvalues::cameraSys[ch].cam_ipaddress,commonvalues::cameraSys[ch].cam_serialNumber);

            int width = (ch == 0) ?  pcamera0->image_width : pcamera1->image_width;
            if( width == 0 ) width = 1920;
            int height = (ch == 0) ?  pcamera0->image_height : pcamera1->image_height;
            if( height == 0) height=1080;

            commonvalues::cameraSys[ch].cam_image_width =  width;
            commonvalues::cameraSys[ch].cam_image_height = height;
            connect(pcamera0,SIGNAL(capture(int,int,QImage*)),this,SLOT(capture(int,int,QImage*)));
            //recog image event
            connect(pcamera0,SIGNAL(recogcapture(int,int)),this,SLOT(recogcapture(int,int)));
            connect(pcamera0,SIGNAL(sigStart(int)),this,SLOT(sigCameraStart(int)));
            //sdw 2016/11/21
            connect(this,SIGNAL(camera_restart(int)),pcamera0,SLOT(restart(int)));

            m_restartcount[ch] = -1;

        }
        else if( ch == 1)
        {
            pcamera1->start(commonvalues::cameraSys[ch].cam_ipaddress,commonvalues::cameraSys[ch].cam_serialNumber);

            int width = (ch == 0) ?  pcamera0->image_width : pcamera1->image_width;
            if( width == 0 ) width = 1920;
            int height = (ch == 0) ?  pcamera0->image_height : pcamera1->image_height;
            if( height == 0) height=1080;

            commonvalues::cameraSys[ch].cam_image_width =  width;
            commonvalues::cameraSys[ch].cam_image_height = height;
            connect(pcamera1,SIGNAL(capture(int,int,QImage*)),this,SLOT(capture(int,int,QImage*)));
            //recog image event
            connect(pcamera1,SIGNAL(recogcapture(int,int)),this,SLOT(recogcapture(int,int)));
            connect(pcamera1,SIGNAL(sigStart(int)),this,SLOT(sigCameraStart(int)));
            //sdw 2016/11/21
            connect(this,SIGNAL(camera_restart(int)),pcamera1,SLOT(restart(int)));

            m_restartcount[ch] = -1;
        }
        QThread::msleep(1000);

    }

}

void MainWindow::init_controlboard()
{
    for(int ch=0 ; ch < commonvalues::cameraChannel; ch++)
    {
        if(ch == 0)
        {
            connect(pliccomm0,SIGNAL(sigReceiveHandler(int,quint8,QString)),this,SLOT(sigLICReceiveHandler(int,quint8,QString)));
            pliccomm0->OpenSerial(true,commonvalues::cameraSys[ch].lic_port,115200,QSerialPort::Data8,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
            pliccomm0->start();
            pliccomm0->SettingREQ_Send();
            pliccomm0->LightREQ_Send();
            pliccomm0->StatusREQ_Send();
            connect(pccucomm0,SIGNAL(sigReceiveHandler(int,quint8,QString,QByteArray)),this,SLOT(sigCCUReceiveHandler(int,quint8,QString,QByteArray)));
            pccucomm0->OpenSerial(true,commonvalues::cameraSys[ch].ccu_port,9600,QSerialPort::Data8,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
            pccucomm0->start();

        }
        else if(ch == 1)
        {
            connect(pliccomm1,SIGNAL(sigReceiveHandler(int,quint8,QString)),this,SLOT(sigLICReceiveHandler(int,quint8,QString)));
            pliccomm1->OpenSerial(true,commonvalues::cameraSys[ch].lic_port,115200,QSerialPort::Data8,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
            pliccomm1->start();
            pliccomm1->SettingREQ_Send();
            pliccomm1->LightREQ_Send();
            pliccomm1->StatusREQ_Send();
            connect(pccucomm1,SIGNAL(sigReceiveHandler(int,quint8,QString,QByteArray)),this,SLOT(sigCCUReceiveHandler(int,quint8,QString,QByteArray)));
            pccucomm1->OpenSerial(true,commonvalues::cameraSys[ch].ccu_port,9600,QSerialPort::Data8,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
            pccucomm1->start();
        }
    }
    ptgmcucomm->OpenSerial(true,commonvalues::tgmcu_port,commonvalues::tgmcu_baudrate,QSerialPort::Data8,QSerialPort::OneStop,QSerialPort::NoParity,QSerialPort::NoFlowControl);
    connect(ptgmcucomm,SIGNAL(sigReceiveHandler(quint8,QString)),this,SLOT(sigTGMCUReceiveHandler(quint8,QString)));
    ptgmcucomm->start();
}

void MainWindow::init_recog()
{
    connect( precogthr,SIGNAL(RecogEndSignal(int,int)),this,SLOT(RecogEndSignal(int,int)));
    precogthr->start();
}

void MainWindow::init_mainthr()
{
    connect(pmainthr,SIGNAL(createcenter(int)),this,SLOT(createcenter(int)));
    connect(pmainthr,SIGNAL(connectcenter(int)),this,SLOT(connectcenter(int)));
    pmainthr->start();
}

void MainWindow::init_autoiris()
{

    if(pautoiris0 != NULL)
    {
        connect(pautoiris0,SIGNAL(SetLICStrobe(int,bool)),this,SLOT(SetLICStrobe(int,bool)));
        pautoiris0->init();
        pautoiris0->start();
    }

    if(pautoiris1 != NULL)
    {
        connect(pautoiris1,SIGNAL(SetLICStrobe(int,bool)),this,SLOT(SetLICStrobe(int,bool)));
        pautoiris1->init();
        pautoiris1->start();
    }


}

void MainWindow::init_database()
{

    connect(pdatabase,SIGNAL(OBUUpdateResult(int,QString,bool,QString)),this,SLOT(OBUUpdateResult(int,QString,bool,QString)));

    pdatabase->init();
    bool brtn = pdatabase->checkcreatedb();
    if(!brtn)
    {
        qDebug() << "Error Database check and create";
    }

    //OBU matching table setting
    if(commonvalues::obuMatch > 0 )pdatabase->setOBUtable();

    pmainthr->pdatabase = pdatabase;
    pdatabase->start();
}

void MainWindow::init_remotevalue()
{
    commonvalues::Remotedata = new clsRemoteDATA(commonvalues::cameraChannel);

    commonvalues::Remotedata->IC_CODE = QString("%1").arg(commonvalues::ManagerID,4,10,QChar('0'));
    commonvalues::Remotedata->LANE_NO = QString("%1").arg(commonvalues::LaneID%100,2,10,QChar('0'));
    commonvalues::Remotedata->BD_NAME = "IPUH_F10";
    commonvalues::Remotedata->MAKER_NAME = "JWIN";
    commonvalues::Remotedata->INTERFACE_VERSION = "1.1";
    commonvalues::Remotedata->FW_H_VERSION = Program_Version;

//    //update check  => FW_UPDATE_RESULT 전송
//    if(commonvalues::firmwareVersion.isEmpty() || commonvalues::firmwareVersion.compare(Program_Version) != 0)
//    {
//        pcfg->set("Firmware","Version",Program_Version);
//        commonvalues::firmwareUpdate = 1;
//    }


    //WORK_START
    if(commonvalues::LandDirection.compare("U") == 0)
    {
        commonvalues::Remotedata->LANE_TYPE = "1";
    }
    else if( commonvalues::LandDirection.compare("D") == 0 )
    {
        commonvalues::Remotedata->LANE_TYPE = "2";
    }
    else if( commonvalues::LandDirection.compare("I") == 0 )
    {
        commonvalues::Remotedata->LANE_TYPE = "4";
    }
    else if( commonvalues::LandDirection.compare("O") == 0 )
    {
        commonvalues::Remotedata->LANE_TYPE = "8";
    }

    commonvalues::Remotedata->LANE_SYS_TYPE = "1";  //hispass

    //LANE_STATUS
    commonvalues::Remotedata->LANE_KIND = "0";
    commonvalues::Remotedata->MANUFACTURER = "진우산전";
    commonvalues::Remotedata->MANU_YEAR = "201912";
    commonvalues::Remotedata->FW_VERSION = Program_Version;
    commonvalues::Remotedata->FW_STATE = "0"; //미등록
    commonvalues::Remotedata->SYS_FORM_TYPE = "1"; //스탠드형
    commonvalues::Remotedata->CONTROLLERCONNECTION_STATE = "정상";
    commonvalues::Remotedata->FTPCONNECTION_STATE = "정상";

    int count = commonvalues::Remotedata->cameraST.count();
    for(int index=0; index < count; index++)
    {
        commonvalues::Remotedata->cameraST[index].CAM_NO = QString("%1").arg(index,2,10,QChar('0'));
        commonvalues::Remotedata->cameraST[index].LANE_INDEX = "1";
        commonvalues::Remotedata->cameraST[index].CAM_POSTION = index == 0 ? "전면" : "후면";
        commonvalues::Remotedata->cameraST[index].IMAGE_RESOLUTION = "1920x1080";
        commonvalues::Remotedata->cameraST[index].ZOOMCONTROL_MAXRANGE = "1023";
        commonvalues::Remotedata->cameraST[index].ZOOMCONTROL_MINRANGE = "1";
        commonvalues::Remotedata->cameraST[index].FOCUSCONTROL_MAXRANGE = "1023";
        commonvalues::Remotedata->cameraST[index].FOCUSCONTROL_MINRANGE = "1";
        commonvalues::Remotedata->cameraST[index].IRISCONTROL_MAXRANGE = "1023";
        commonvalues::Remotedata->cameraST[index].IRISCONTROL_MINRANGE = "1";
        commonvalues::Remotedata->cameraST[index].CAMERA_STATE = "정상";
        commonvalues::Remotedata->cameraST[index].LENSCONTROLBOARD_STATE = "정상";
        commonvalues::Remotedata->cameraST[index].DETECTOR_STATE = "정상";
        commonvalues::Remotedata->cameraST[index].STROBE_STATE = "켜짐";

    }
}

void MainWindow::applyconfig2common()
{
    QString svalue;
    bool bvalue;
    uint uivalue;
    float fvalue;

    if( pcfg->getuint("CameraChannel","Count",&uivalue)) {  commonvalues::cameraChannel = uivalue; }

    //camera setting values
    for(int ch = 0; ch < CAMERASYSTEM_MAXCHANNEL; ch++)
    {
        QString strchannel = QString("Channel%1").arg(ch);
        /*         CameraConfig              */
        QString strtitle = QString("%1|Camera").arg(strchannel);
        if( ( svalue = pcfg->get(strtitle,"IP") ) != NULL){ commonvalues::cameraSys[ch].cam_ipaddress = svalue; }
        if( pcfg->getuint(strtitle,"SerialNumber",&uivalue)) {  commonvalues::cameraSys[ch].cam_serialNumber = uivalue; }
//        if( pcfg->getuint(strtitle,"Bright",&uivalue)) { commonvalues::cameraSys[ch].cam_brightness = uivalue; }
//        if( pcfg->getbool(strtitle.arg(strchannel),"Bright_AutoMode",&bvalue)) { commonvalues::cameraSys[ch].cam_brightness_auto = bvalue; }
        if( pcfg->getfloat(strtitle,"Shutter",&fvalue)) { commonvalues::cameraSys[ch].cam_shutter = fvalue; }
//        if( pcfg->getbool(strtitle,"Shutter_AutoMode",&bvalue)) { commonvalues::cameraSys[ch].cam_shutter_auto = bvalue; }
        if( pcfg->getfloat(strtitle,"Gain",&fvalue)) { commonvalues::cameraSys[ch].cam_gain = fvalue; }
//        if( pcfg->getbool(strtitle,"Gain_AutoMode",&bvalue)) { commonvalues::cameraSys[ch].cam_gain_auto = bvalue; }
//        if( pcfg->getuint(strtitle,"Sharpness",&uivalue)) { commonvalues::cameraSys[ch].cam_sharpness = uivalue; }
//        if( pcfg->getuint(strtitle,"Hue",&uivalue)) { commonvalues::cameraSys[ch].cam_hue = uivalue; }
//        if( pcfg->getuint(strtitle,"Saturation",&uivalue)) { commonvalues::cameraSys[ch].cam_saturation = uivalue; }
//        if( pcfg->getuint(strtitle,"Gamma",&uivalue)) { commonvalues::cameraSys[ch].cam_gamma = uivalue; }
        if( pcfg->getfloat(strtitle,"WBRed",&fvalue)) { commonvalues::cameraSys[ch].cam_whitebalance_red = fvalue; }
        if( pcfg->getfloat(strtitle,"WBBlue",&fvalue)) { commonvalues::cameraSys[ch].cam_whitebalance_blue = fvalue; }
//        if( pcfg->getbool(strtitle,"WB_AutoMode",&bvalue)) { commonvalues::cameraSys[ch].cam_whitebalance_auto = bvalue; }
//        if( pcfg->getbool(strtitle,"WB_OnOff",&bvalue)) { commonvalues::cameraSys[ch].cam_whitebalance_onoff = bvalue; }
        if( pcfg->getuint(strtitle,"Strobe_Polarity",&uivalue)) { commonvalues::cameraSys[ch].cam_strobepolarity = uivalue; }

        /*        Autoiris             */
        //basic
        strtitle = QString("%1|AutoIris|Basic").arg(strchannel);
        if( pcfg->getbool(strtitle,"Use",&bvalue)) { commonvalues::autoirisinfo[ch].autoiris_use = bvalue; }
        if( pcfg->getuint(strtitle,"StandardBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_standardbr = uivalue; }
        if( pcfg->getuint(strtitle,"BrRange",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_brrange = uivalue; }
        if( pcfg->getuint(strtitle,"BrInterval",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_brinterval = uivalue; }
        if( pcfg->getuint(strtitle,"BrAvgCount",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_bravgcount = uivalue; }
        if( pcfg->getfloat(strtitle,"MaxShutter",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_maxshutter = fvalue; }
        if( pcfg->getfloat(strtitle,"MinShutter",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_minshutter = fvalue; }
        if( pcfg->getfloat(strtitle,"ShutterStep",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_shutterstep = fvalue; }
        if( pcfg->getfloat(strtitle,"MaxGain",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_maxgain = fvalue; }
        if( pcfg->getfloat(strtitle,"MinGain",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_mingain = fvalue; }
        if( pcfg->getfloat(strtitle,"GainStep",&fvalue)) {  commonvalues::autoirisinfo[ch].autoiris_gainstep = fvalue; }
        if( pcfg->getbool(strtitle,"Debug",&bvalue)) { commonvalues::autoirisinfo[ch].autoiris_debug = bvalue; }
        //recflect&back
        strtitle = QString("%1|AutoIris|Reflection&Back").arg(strchannel);
        if( pcfg->getbool(strtitle,"Reflection",&bvalue)) { commonvalues::autoirisinfo[ch].autoiris_reflection = bvalue; }
        if( pcfg->getbool(strtitle,"Backlight",&bvalue)) { commonvalues::autoirisinfo[ch].autoiris_backlight = bvalue; }
        if( pcfg->getuint(strtitle,"MaxBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_maxbr = uivalue; }
        if( pcfg->getuint(strtitle,"MinBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_minbr = uivalue; }
        if( pcfg->getuint(strtitle,"BrStep",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_brstep = uivalue; }
        if( pcfg->getuint(strtitle,"MaxPlateBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_maxplatebr = uivalue; }
        if( pcfg->getuint(strtitle,"MinPlateBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_minplatebr = uivalue; }
        if( pcfg->getuint(strtitle,"BrChangeCount",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_brchangecount  = uivalue; }
        if( pcfg->getuint(strtitle,"Mode",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_reflectbackmode = uivalue; }
        //no Recog
        strtitle = QString("%1|AutoIris|NoRecog").arg(strchannel);
        if( pcfg->getbool(strtitle,"Use",&bvalue)) { commonvalues::autoirisinfo[ch].autoiris_norecog = bvalue; }
        if( pcfg->getuint(strtitle,"MaxBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_norecogmaxbr = uivalue; }
        if( pcfg->getuint(strtitle,"MinBr",&uivalue)) {  commonvalues::autoirisinfo[ch].autoiris_norecogminbr = uivalue; }

        /*        Light             */
        strtitle = QString("%1|Light").arg(strchannel);
        if( pcfg->getbool(strtitle,"24H",&bvalue)) { commonvalues::autolightnfo[ch].light_24h = bvalue; }
        if( pcfg->getfloat(strtitle,"Shutter",&fvalue)) {  commonvalues::autolightnfo[ch].light_shutter = fvalue; }
        if( pcfg->getfloat(strtitle,"OnGain",&fvalue)) {  commonvalues::autolightnfo[ch].light_ongain = fvalue; }
        if( pcfg->getfloat(strtitle,"OffGain",&fvalue)) {  commonvalues::autolightnfo[ch].light_offgain = fvalue; }
        /*       controlboard       */
        if( ( svalue = pcfg->get(QString("%1|LIC").arg(strchannel),"Port") ) != NULL){ commonvalues::cameraSys[ch].lic_port = svalue; }
        if( ( svalue = pcfg->get(QString("%1|CCU").arg(strchannel),"Port") ) != NULL){ commonvalues::cameraSys[ch].ccu_port = svalue; }
        if( pcfg->getuint(QString("%1|CCU").arg(strchannel),"VioREPTimeout",&uivalue)) {  commonvalues::cameraSys[ch].ccu_viorepTimout = uivalue; }
        if( pcfg->getuint(QString("%1|TGMCU").arg(strchannel),"TGLane",&uivalue)) {  commonvalues::cameraSys[ch].TGLane = uivalue; }


        /*      Recognition             */
        strtitle = QString("%1|RECOGNITION|PLATEAREA").arg(strchannel);
        if( pcfg->getuint(strtitle,"sx",&uivalue)) { commonvalues::cameraSys[ch].recog_plateroi_sx = uivalue; }
        if( pcfg->getuint(strtitle,"sy",&uivalue)) { commonvalues::cameraSys[ch].recog_plateroi_sy = uivalue; }
        if( pcfg->getuint(strtitle,"width",&uivalue)) { commonvalues::cameraSys[ch].recog_plateroi_width = uivalue; }
        if( pcfg->getuint(strtitle,"height",&uivalue)) { commonvalues::cameraSys[ch].recog_plateroi_height = uivalue; }
        strtitle = QString("%1|RECOGNITION|BRAREA").arg(strchannel);
        if( pcfg->getuint(strtitle,"sx",&uivalue)) { commonvalues::cameraSys[ch].recog_brroi_sx = uivalue; }
        if( pcfg->getuint(strtitle,"sy",&uivalue)) { commonvalues::cameraSys[ch].recog_brroi_sy = uivalue; }
        if( pcfg->getuint(strtitle,"width",&uivalue)) { commonvalues::cameraSys[ch].recog_brroi_width = uivalue; }
        if( pcfg->getuint(strtitle,"height",&uivalue)) { commonvalues::cameraSys[ch].recog_brroi_height = uivalue; }
        strtitle = QString("%1|RECOGNITION").arg(strchannel);
        if( pcfg->getuint(strtitle,"VehiclePosition",&uivalue)) { commonvalues::cameraSys[ch].recog_vehicleposition = uivalue; }
        if( pcfg->getuint(strtitle,"RecognitionMode",&uivalue)) { commonvalues::cameraSys[ch].recog_recognition_mode = uivalue; }
        if( ( svalue = pcfg->get(strtitle,"RecogSeq")) != NULL )
        {
                int count = svalue.length();
                if( count > 8) count = 8;
                for(int i=0; i<count; i++)
                {
                  commonvalues::cameraSys[ch].recog_recogseq[i] =  svalue[i].digitValue();
                }
        }
        if( pcfg->getuint(strtitle,"RepeatTime",&uivalue)) { commonvalues::cameraSys[ch].recog_recogrepeattime = uivalue; }
        if( pcfg->getuint(QString("%1|RECOGNITION|RAWSAVE").arg(strchannel),"Type",&uivalue)) { commonvalues::cameraSys[ch].recog_rawsavetype = uivalue; }
        if( pcfg->getuint(QString("%1|RECOGNITION|RAWSAVE").arg(strchannel),"COUNT",&uivalue)) { commonvalues::cameraSys[ch].recog_rawsavecount = uivalue; }

    }

    if( pcfg->getuint("Confirm","ConfirmType",&uivalue)) {  commonvalues::confirmSysType = uivalue; }
    if( pcfg->getuint("CCU","seqincreaseType",&uivalue)) {  commonvalues::seqincreaseType = uivalue; }
    if( pcfg->getuint("CCU","CCUType",&uivalue)) {  commonvalues::ccutype = uivalue; }
    if( pcfg->getuint("CCU","CCUProtocolType",&uivalue)) {  commonvalues::ccuprotocol = uivalue; }
    if( pcfg->getuint("CCU","CCUVioDelay",&uivalue)) {  commonvalues::ccuViodelay = uivalue; }
    if( pcfg->getuint("CCU","CCUSendSleep",&uivalue)) {  commonvalues::ccuSendsleep = uivalue; }
    if( pcfg->getuint("CCU","DayChange",&uivalue)) {  commonvalues::daychange = uivalue; }
    if( pcfg->getuint("CCU","SyncIgnore",&uivalue)) {  commonvalues::syncIgnore = uivalue; }
    if( pcfg->getuint("CCU","OBUMatch",&uivalue)) {  commonvalues::obuMatch = uivalue; }
    if( pcfg->getuint("CCU","OBUMode",&uivalue)) {  commonvalues::obuMode = uivalue; }


    /*        Center  value            */
   //static QList<centerinfo> center_list;
//   if( pcfg->getuint("CENTER","Count",&uivalue)) { commonvalues::center_count = uivalue; }
//   for(int i=0; i < commonvalues::center_count; i++)
    {
        int i=0;
        QString title = "CENTER|LIST" + QString::number(i);
        while( ( svalue = pcfg->get(title,"IP") ) != NULL)
        {
            CenterInfo centerinfo;
            if( ( svalue = pcfg->get(title,"CenterName") ) != NULL ) { centerinfo.centername = svalue; }
            if( ( svalue = pcfg->get(title,"IP") ) != NULL ) { centerinfo.ip = svalue; }
            if( pcfg->getuint(title,"TCPPort",&uivalue)) { centerinfo.tcpport = uivalue; }
            if( pcfg->getuint(title,"AliveInterval",&uivalue)) { centerinfo.connectioninterval = uivalue; }
            if( pcfg->getuint(title,"ProtocolType",&uivalue)) { centerinfo.protocol_type = uivalue; }
            if( pcfg->getuint(title,"FTPPort",&uivalue)) { centerinfo.ftpport = uivalue; }
            if( ( svalue = pcfg->get(title,"FTPUserID") ) != NULL ) { centerinfo.userID = svalue; }
            if( ( svalue = pcfg->get(title,"FTPPassword") ) != NULL ) { centerinfo.password = svalue; }
            if( ( svalue = pcfg->get(title,"FTPPath") ) != NULL ) { centerinfo.ftpPath = svalue; }
            if( pcfg->getuint(title,"FileNameSelect",&uivalue)) { centerinfo.fileNameSelect = uivalue; }

            centerinfo.plblstatus = new QLabel();
            centerinfo.plblstatus->setText( QString("%1:%2")
                        .arg(centerinfo.ip).arg(centerinfo.tcpport));
            centerinfo.plblstatus->setStyleSheet("QLabel { background-color : red; }");

            commonvalues::center_list.append(centerinfo);

            i++;
            title = "CENTER|LIST" + QString::number(i);
        }
    }
    if( pcfg->getuint("CENTER","FTPRetry",&uivalue)) { commonvalues::ftpretry = uivalue; }
    if( ( svalue = pcfg->get("CENTER","FTPSavePath") ) != NULL){ commonvalues::FTPSavePath = svalue; }

    if( ( svalue = pcfg->get(QString("TGMCU"),"Port") ) != NULL){ commonvalues::tgmcu_port = svalue; }
    if( pcfg->getuint(QString("TGMCU"),"Baudrate",&uivalue)) { commonvalues::tgmcu_baudrate = uivalue; }

    if( pcfg->getuint("Local","LaneID",&uivalue)) { commonvalues::LaneID = uivalue; }
    if( pcfg->getuint("Local","ManagerID",&uivalue)) { commonvalues::ManagerID = uivalue; }
    if( ( svalue = pcfg->get("Local","LandDirection") ) != NULL){ commonvalues::LandDirection = svalue; }


 /*        Database  value            */
   if( ( svalue = pcfg->get("DATABASE","IP") ) != NULL){ commonvalues::databaseinfo.db_ip = svalue; }
   //commonvalues::db_ip = "127.0.0.1";
   if( pcfg->getuint("DATABASE","Port",&uivalue)) { commonvalues::databaseinfo.db_port = uivalue; }
   //commonvalues::db_port = 3306;
   if( ( svalue = pcfg->get("DATABASE","Name") ) != NULL){ commonvalues::databaseinfo.db_name = svalue; }
   //commonvalues::db_name = "lprdb";
   if( ( svalue = pcfg->get("DATABASE","User") ) != NULL){ commonvalues::databaseinfo.db_user = svalue; }
   //commonvalues::db_user = "root";
   if( ( svalue = pcfg->get("DATABASE","Password") ) != NULL){ commonvalues::databaseinfo.db_password = svalue; }
   //commonvalues::db_password = "ubuntu";
   if( pcfg->getuint("DATABASE","ConnectionTimeout",&uivalue)) { commonvalues::databaseinfo.db_connectionTimeout = uivalue; }
   if( pcfg->getuint("DATABASE","CommandTimeout",&uivalue)) { commonvalues::databaseinfo.db_commandTimeout = uivalue; }
   if( pcfg->getuint("DATABASE","StorageDuration",&uivalue)) { commonvalues::databaseinfo.db_storageDuration = uivalue; }

  /*       Log Level value             */
   if( pcfg->getuint("LOG","Level",&uivalue)) { commonvalues::loglevel = uivalue; }
   if( pcfg->getuint("LOG","RecogSaveLog",&uivalue)) { commonvalues::recogSaveLog = uivalue; }

   /*       Firmware Update     */
   if( pcfg->getuint("FIRMWARE","Update",&uivalue)) { commonvalues::fw_update = uivalue; }
   if( ( svalue = pcfg->get("FIRMWARE","ControlNo") ) != NULL){ commonvalues::fw_controlNo = svalue; }
   if( ( svalue = pcfg->get("FIRMWARE","FileName") ) != NULL){ commonvalues::fw_filename = svalue; }
   if( ( svalue = pcfg->get("FIRMWARE","ResultTime") ) != NULL){ commonvalues::fw_resultTime = svalue; }

}

void MainWindow::applycommon2config()
{
    pcfg->setuint("CameraChannel","Count",commonvalues::cameraChannel);

    for(int ch = 0; ch < CAMERASYSTEM_MAXCHANNEL; ch++)
    {
        QString strchannel = QString("Channel%1").arg(ch);
        /*         CameraConfig              */
        QString strtitle = QString("%1|Camera").arg(strchannel);
        if(ch == 1)
            commonvalues::cameraSys[ch].cam_ipaddress = "169.253.1";
        else
            commonvalues::cameraSys[ch].cam_ipaddress = "169.254.0.1";
        pcfg->set(strtitle,"IP",commonvalues::cameraSys[ch].cam_ipaddress);
        pcfg->setuint(strtitle,"SerialNumber",commonvalues::cameraSys[ch].cam_serialNumber);
//        pcfg->setuint(strtitle,"Bright",commonvalues::cameraSys[ch].cam_brightness);
//        pcfg->setbool(strtitle.arg(strchannel),"Bright_AutoMode",commonvalues::cameraSys[ch].cam_brightness_auto);
        pcfg->setuint(strtitle,"Shutter",commonvalues::cameraSys[ch].cam_shutter);
//        pcfg->setbool(strtitle,"Shutter_AutoMode",commonvalues::cameraSys[ch].cam_shutter_auto);
        pcfg->setuint(strtitle,"Gain",commonvalues::cameraSys[ch].cam_gain);
//        pcfg->setbool(strtitle,"Gain_AutoMode",commonvalues::cameraSys[ch].cam_gain_auto);
//        pcfg->setuint(strtitle,"Sharpness",commonvalues::cameraSys[ch].cam_sharpness);
//        pcfg->setuint(strtitle,"Hue",commonvalues::cameraSys[ch].cam_hue);
//        pcfg->setuint(strtitle,"Saturation",commonvalues::cameraSys[ch].cam_saturation);
//        pcfg->setuint(strtitle,"Gamma",commonvalues::cameraSys[ch].cam_gamma);
        pcfg->setuint(strtitle,"WBRed",commonvalues::cameraSys[ch].cam_whitebalance_red);
        pcfg->setuint(strtitle,"WBBlue",commonvalues::cameraSys[ch].cam_whitebalance_blue);
//        pcfg->setbool(strtitle,"WB_AutoMode",commonvalues::cameraSys[ch].cam_whitebalance_auto);
//        pcfg->setbool(strtitle,"WB_OnOff",commonvalues::cameraSys[ch].cam_whitebalance_onoff);
        pcfg->setuint(strtitle,"Strobe_Polarity",commonvalues::cameraSys[ch].cam_strobepolarity);

        /*        Autoiris             */
        //basic
        strtitle = QString("%1|AutoIris|Basic").arg(strchannel);
        pcfg->setbool(strtitle,"Use",commonvalues::autoirisinfo[ch].autoiris_use);
        pcfg->setuint(strtitle,"StandardBr",commonvalues::autoirisinfo[ch].autoiris_standardbr);
        pcfg->setuint(strtitle,"BrRange",commonvalues::autoirisinfo[ch].autoiris_brrange);
        pcfg->setuint(strtitle,"BrInterval",commonvalues::autoirisinfo[ch].autoiris_brinterval);
        pcfg->setuint(strtitle,"BrAvgCount",commonvalues::autoirisinfo[ch].autoiris_bravgcount);
        pcfg->setfloat(strtitle,"MaxShutter",commonvalues::autoirisinfo[ch].autoiris_maxshutter);
        pcfg->setfloat(strtitle,"MinShutter",commonvalues::autoirisinfo[ch].autoiris_minshutter );
        pcfg->setfloat(strtitle,"ShutterStep",commonvalues::autoirisinfo[ch].autoiris_shutterstep);
        pcfg->setfloat(strtitle,"MaxGain",commonvalues::autoirisinfo[ch].autoiris_maxgain);
        pcfg->setfloat(strtitle,"MinGain",commonvalues::autoirisinfo[ch].autoiris_mingain);
        pcfg->setfloat(strtitle,"GainStep",commonvalues::autoirisinfo[ch].autoiris_gainstep);
        pcfg->setbool(strtitle,"Debug",commonvalues::autoirisinfo[ch].autoiris_debug);
        //recflect&back
        strtitle = QString("%1|AutoIris|Reflection&Back").arg(strchannel);
        pcfg->setbool(strtitle,"Reflection",commonvalues::autoirisinfo[ch].autoiris_reflection);
        pcfg->setbool(strtitle,"Backlight",commonvalues::autoirisinfo[ch].autoiris_backlight);
        pcfg->setuint(strtitle,"MaxBr",commonvalues::autoirisinfo[ch].autoiris_maxbr);
        pcfg->setuint(strtitle,"MinBr",commonvalues::autoirisinfo[ch].autoiris_minbr);
        pcfg->setuint(strtitle,"BrStep",commonvalues::autoirisinfo[ch].autoiris_brstep);
        pcfg->setuint(strtitle,"MaxPlateBr",commonvalues::autoirisinfo[ch].autoiris_maxplatebr);
        pcfg->setuint(strtitle,"MinPlateBr",commonvalues::autoirisinfo[ch].autoiris_minplatebr);
        pcfg->setuint(strtitle,"BrChangeCount",commonvalues::autoirisinfo[ch].autoiris_brchangecount);
        pcfg->setuint(strtitle,"Mode",commonvalues::autoirisinfo[ch].autoiris_reflectbackmode);
        //no Recog
        strtitle = QString("%1|AutoIris|NoRecog").arg(strchannel);
        pcfg->setbool(strtitle,"Use",commonvalues::autoirisinfo[ch].autoiris_norecog);
        pcfg->setuint(strtitle,"MaxBr",commonvalues::autoirisinfo[ch].autoiris_norecogmaxbr);
        pcfg->setuint(strtitle,"MinBr",commonvalues::autoirisinfo[ch].autoiris_norecogminbr);

        /*        Light             */
        strtitle = QString("%1|Light").arg(strchannel);
        pcfg->setbool(strtitle,"24H",commonvalues::autolightnfo[ch].light_24h);
        pcfg->setfloat(strtitle,"Shutter",commonvalues::autolightnfo[ch].light_shutter);
        pcfg->setfloat(strtitle,"OnGain",commonvalues::autolightnfo[ch].light_ongain);
        pcfg->setfloat(strtitle,"OffGain",commonvalues::autolightnfo[ch].light_offgain);
        /*       controlboard       */
        pcfg->set(QString("%1|LIC").arg(strchannel),"Port",commonvalues::cameraSys[ch].lic_port);
        pcfg->set(QString("%1|CCU").arg(strchannel),"Port",commonvalues::cameraSys[ch].ccu_port);
        pcfg->setuint(QString("%1|CCU").arg(strchannel),"VioREPTimeout",commonvalues::cameraSys[ch].ccu_viorepTimout);
        pcfg->setuint(QString("%1|TGMCU").arg(strchannel),"TGLane",commonvalues::cameraSys[ch].TGLane);

        pcfg->setuint(QString("Confirm"),"ConfirmType",commonvalues::confirmSysType);
        pcfg->setuint(QString("CCU"),"seqincreaseType",commonvalues::seqincreaseType);
        pcfg->setuint(QString("CCU"),"CCUType",commonvalues::ccutype);
        pcfg->setuint(QString("CCU"),"CCUProtocolType",commonvalues::ccuprotocol);
        pcfg->setuint(QString("CCU"),"CCUVioDelay",commonvalues::ccuViodelay);
        pcfg->setuint(QString("CCU"),"CCUSendSleep",commonvalues::ccuSendsleep);
        pcfg->setuint(QString("CCU"),"DayChange",commonvalues::daychange);
        pcfg->setuint(QString("CCU"),"SyncIgnore",commonvalues::syncIgnore);
        pcfg->setuint(QString("CCU"),"OBUMatch",commonvalues::obuMatch);
        pcfg->setuint(QString("CCU"),"OBUMode",commonvalues::obuMode);

        /*      Recognition             */
        strtitle = QString("%1|RECOGNITION|PLATEAREA").arg(strchannel);
        pcfg->setuint(strtitle,"sx",commonvalues::cameraSys[ch].recog_plateroi_sx);
        pcfg->setuint(strtitle,"sy",commonvalues::cameraSys[ch].recog_plateroi_sy);
        pcfg->setuint(strtitle,"width",commonvalues::cameraSys[ch].recog_plateroi_width);
        pcfg->setuint(strtitle,"height",commonvalues::cameraSys[ch].recog_plateroi_height);
        strtitle = QString("%1|RECOGNITION|BRAREA").arg(strchannel);
        pcfg->setuint(strtitle,"sx",commonvalues::cameraSys[ch].recog_brroi_sx);
        pcfg->setuint(strtitle,"sy",commonvalues::cameraSys[ch].recog_brroi_sy);
        pcfg->setuint(strtitle,"width",commonvalues::cameraSys[ch].recog_brroi_width);
        pcfg->setuint(strtitle,"height",commonvalues::cameraSys[ch].recog_brroi_height);
        strtitle = QString("%1|RECOGNITION").arg(strchannel);
        pcfg->setuint(strtitle,"VehiclePosition",commonvalues::cameraSys[ch].recog_vehicleposition);
        pcfg->setuint(strtitle,"RecognitionMode",commonvalues::cameraSys[ch].recog_recognition_mode);
        {
            int count = sizeof( commonvalues::cameraSys[ch].recog_recogseq ) / sizeof( int );
            QString strval;

            for(int i=0; i< count;i++)
            {
                strval += QString::number(commonvalues::cameraSys[ch].recog_recogseq[i]);
            }
            pcfg->set(strtitle,"RecogSeq",strval);
        }
        pcfg->setuint(strtitle,"RepeatTime",commonvalues::cameraSys[ch].recog_recogrepeattime);
        pcfg->setuint(QString("%1|RECOGNITION|RAWSAVE").arg(strchannel),"Type",commonvalues::cameraSys[ch].recog_rawsavetype);
        pcfg->setuint(QString("%1|RECOGNITION|RAWSAVE").arg(strchannel),"COUNT",commonvalues::cameraSys[ch].recog_rawsavecount);
    }

    /*        Center  value            */

   //cfg->setuint("CENTER","Count",commonvalues::center_count);
   //pcfg->setbool("CENTER","Backup",commonvalues::backuptrans);
   {
       CenterInfo cinfo;
       cinfo.centername = "center";
       cinfo.ip = "192.168.10.101";
       cinfo.tcpport = 31001;
       cinfo.connectioninterval = 1800;
       cinfo.tcpport = 21;
       cinfo.userID = "tes";
       cinfo.password = "tes";
       cinfo.ftpPath = "";
       cinfo.plblstatus = new QLabel();
       cinfo.plblstatus->setText( QString("%1:%2")
                   .arg(cinfo.ip).arg(cinfo.tcpport));
       cinfo.plblstatus->setStyleSheet("QLabel { background-color : red; }");
       commonvalues::center_list.append(cinfo);

       QString title = "CENTER|LIST0";
       pcfg->set(title,"CenterName",cinfo.centername);
       pcfg->set(title,"IP",cinfo.ip);
       pcfg->setuint(title,"TCPPort",cinfo.tcpport);
       pcfg->setuint(title,"AliveInterval",cinfo.connectioninterval);
       pcfg->setuint(title,"ProtocolType",cinfo.protocol_type);
       pcfg->setuint(title,"FTPPort",cinfo.ftpport);
       pcfg->set(title,"FTPUserID",cinfo.userID);
       pcfg->set(title,"FTPPassword",cinfo.password);
       pcfg->set(title,"FTPPath",cinfo.ftpPath);
       pcfg->setuint(title,"FileNameSelect",cinfo.fileNameSelect);

   }
    pcfg->setuint("CENTER","FTPRetry",commonvalues::ftpretry);
    pcfg->set("CENTER","FTPSavePath",commonvalues::FTPSavePath);


    pcfg->set(QString("TGMCU"),"Port",commonvalues::tgmcu_port);
    pcfg->setuint(QString("TGMCU"),"Baudrate",commonvalues::tgmcu_baudrate);

    pcfg->setuint("Local","LaneID",commonvalues::LaneID);
    pcfg->setuint("Local","ManagerID",commonvalues::ManagerID);
    pcfg->set("Local","LandDirection",commonvalues::LandDirection);

     /*        Database  value            */
    pcfg->set("DATABASE","IP",commonvalues::databaseinfo.db_ip);
    //commonvalues::db_ip = "127.0.0.1";
    pcfg->setuint("DATABASE","Port",commonvalues::databaseinfo.db_port);
    //commonvalues::db_port = 3306;
    pcfg->set("DATABASE","Name",commonvalues::databaseinfo.db_name);
    pcfg->set("DATABASE","User",commonvalues::databaseinfo.db_user);
    //commonvalues::db_user = "root";
    pcfg->set("DATABASE","Password",commonvalues::databaseinfo.db_password);
    //commonvalues::db_password = "ubuntu";
    pcfg->setuint("DATABASE","ConnectionTimeout",commonvalues::databaseinfo.db_connectionTimeout);
    pcfg->setuint("DATABASE","CommandTimeout",commonvalues::databaseinfo.db_commandTimeout);
    pcfg->setuint("DATABASE","StorageDuration",commonvalues::databaseinfo.db_storageDuration);


      /*       Log Level value             */
    pcfg->setuint("LOG","Level",(uint)commonvalues::loglevel );
    pcfg->setuint("LOG","RecogSaveLog",(uint)commonvalues::recogSaveLog );
}

void MainWindow::checkcenterstatus()
{
    int count = commonvalues::center_list.size();

    if( commonvalues::center_count != count )
    {
        if(commonvalues::center_count < count)
        {
            for(int i=commonvalues::center_count; i < count; i++)
            {
                ui->statusBar->addWidget(commonvalues::center_list.value(i).plblstatus);
            }

        }
        commonvalues::center_count = count;
    }

    count = commonvalues::center_list.size();
    for(int i = 0; i < count; i++)
    {
        commonvalues::center_list.value(i).plblstatus->setText( QString("%1:%2")
                                        .arg(commonvalues::center_list.at(i).ip).arg(commonvalues::center_list.value(i).tcpport));
        if(commonvalues::center_list.value(i).status )
        {
            commonvalues::center_list.value(i).plblstatus->setStyleSheet("QLabel { background-color : green; }");

        }
        else
        {
            commonvalues::center_list.value(i).plblstatus->setStyleSheet("QLabel { background-color : red; }");
        }
    }
}

void MainWindow::loglevelsetting(int loglevel)
{

    //class
    if( pcamera0 != NULL) {
        pcamera0->m_loglevel = loglevel;
        pcamera0->log->m_loglevel = loglevel;
    }
    if(pcamera1 != NULL) {
        pcamera1->m_loglevel = loglevel;
        pcamera1->log->m_loglevel = loglevel;
    }
    if(pliccomm0 != NULL) {
        pliccomm0->m_loglevel = loglevel;
        pliccomm0->log->m_loglevel = loglevel;
    }
    if(pliccomm1 != NULL) {
        pliccomm1->m_loglevel = loglevel;
        pliccomm1->log->m_loglevel = loglevel;
    }
    if(pccucomm0 != NULL) {
        pccucomm0->m_loglevel = loglevel;
        pccucomm0->log->m_loglevel = loglevel;
    }
    if(pccucomm1 != NULL) {
        pccucomm1->m_loglevel = loglevel;
        pccucomm1->log->m_loglevel = loglevel;
    }
    if(ptgmcucomm != NULL) {
        ptgmcucomm->m_loglevel = loglevel;
        ptgmcucomm->log->m_loglevel = loglevel;
    }
    if( precogthr != NULL) {
        precogthr->m_loglevel = loglevel;
        precogthr->log->m_loglevel = loglevel;
    }
    if(pmainthr != NULL) {
        pmainthr->m_loglevel = loglevel;
        pmainthr->log->m_loglevel = loglevel;
    }
    if(pdatabase != NULL) {
        pdatabase->m_loglevel = loglevel;
        pdatabase->log->m_loglevel = loglevel;
    }
    if(pautoiris0 != NULL) {
        pautoiris0->m_loglevel = loglevel;
        pautoiris0->log->m_loglevel = loglevel;
    }
    if(pautoiris1 != NULL) {
        pautoiris1->m_loglevel = loglevel;
        pautoiris1->log->m_loglevel = loglevel;
    }
    if(plog != NULL) {
        plog->m_loglevel = loglevel;
    }

    int client_count = commonvalues::clientlist.size();
    for(int i=0; i < client_count; i++)
    {
        commonvalues::clientlist.value(i)->m_loglevel = loglevel;
        commonvalues::clientlist.value(i)->log->m_loglevel = loglevel;
    }

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_F11)
    {
        loginsetting(!m_blogin);
    }
}

void MainWindow::cameraConfigview0()
{
    pcameraconfigdlg0->show();
    pcameraconfigdlg0->raise();
}

void MainWindow::apsgdlgview0()
{
    papsgdlg0->show();
    papsgdlg0->raise();
}

void MainWindow::controlboarddlgview0()
{
    pcontrolboarddlg0->show();
    pcontrolboarddlg0->raise();
}

void MainWindow::recognitiondlgview0()
{
    precognitiondlg0->show();
    precognitiondlg0->raise();
}

void MainWindow::logdlgview0()
{
    plogdlg0->show();
    plogdlg0->raise();
}

void MainWindow::cameraConfigview1()
{
    pcameraconfigdlg1->show();
    pcameraconfigdlg1->raise();
}

void MainWindow::apsgdlgview1()
{
    papsgdlg1->show();
    papsgdlg1->raise();
}

void MainWindow::controlboarddlgview1()
{
    pcontrolboarddlg1->show();
    pcontrolboarddlg1->raise();
}

void MainWindow::recognitiondlgview1()
{
    precognitiondlg1->show();
    precognitiondlg1->raise();
}

void MainWindow::logdlgview1()
{
    plogdlg1->show();
    plogdlg1->raise();
}

void MainWindow::centerdlgview()
{
    pcenterdlg->show();
    pcenterdlg->raise();
}

void MainWindow::databasdlgeview()
{
    pdatabasedlg->show();
    pdatabasedlg->raise();
}

void MainWindow::configurationdlgview()
{
    pconfigdlg->show();
    pconfigdlg->raise();
}

void MainWindow::sigLICReceiveHandler(int channel, quint8 Cmd, QString data)
{
    QString logstr;

    pmainthr->m_isignalflag |= mainthread::SIG_LICRcv;

    LogDlg *plogdlg;
    LICComm *pliccomm;
    if(channel == 0)
    {
        plogdlg = plogdlg0;
        pliccomm = pliccomm0;
    }
    else
    {
        plogdlg = plogdlg1;
        pliccomm = pliccomm1;
    }


    if(!data.isNull())
    {
        if(data.contains("NAK"))
        {
            logstr = QString("LIC CMD:%1, DATA:%2").arg(Cmd).arg(data);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
            return;
        }
        else if(data.contains("ACK"))
            return;
    }


    if(Cmd == LICComm::CMD_Connected)
    {
        logstr = QString("LIC Connected(ch%1)").arg(channel);
        plogdlg->logappend(LogDlg::loglic,logstr);

        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
    }
    else if(Cmd == LICComm::CMD_Disconnected)
    {
        logstr = QString("LIC Disconnected(ch%1)").arg(channel);
        //plogdlg->logappend(LogDlg::loglic,logstr);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
    }
    else if(Cmd == LICComm::CMD_SettingREQ)
    {
        emit BoardDlgTextUpdate(channel,ControlBoardDlg::boardlic,Cmd,data);
    }
    else if(Cmd == LICComm::CMD_StatusREQ)
    {
        emit BoardDlgTextUpdate(channel,ControlBoardDlg::boardlic,Cmd,data);
        // lic reset -> SettingTRANSFER_Send  X
        QStringList datalist = data.split(',');

        commonvalues::currentvalue[channel].cur_cds = (int)QString(datalist[6]).toUInt();
        commonvalues::currentvalue[channel].cur_temp = (int)pliccomm->StatusINFO.Temperature;
        commonvalues::currentvalue[channel].cur_zoom = (int)pliccomm->StatusINFO.Zoom;
        commonvalues::currentvalue[channel].cur_focus = (int)pliccomm->StatusINFO.Focus;
        commonvalues::currentvalue[channel].cur_iris = (int)pliccomm->StatusINFO.Iris;

        commonvalues::localdevstatus[channel].Humidity = pliccomm->StatusINFO.Humidity;
        commonvalues::localdevstatus[channel].fan =  pliccomm->StatusINFO.FAN;
        commonvalues::localdevstatus[channel].heater = pliccomm->StatusINFO.HEATER;
        commonvalues::localdevstatus[channel].door = pliccomm->StatusINFO.DoorStatus;
    }
    else if(Cmd == LICComm::CMD_LightREQ)
    {
        QStringList datalist = data.split(',');
        if( datalist.count() >= 3 )
        {
            if(pliccomm->lightINFO.ONOFF > 0 && pliccomm->lightINFO.Status == 0)
                commonvalues::localdevstatus[channel].light = 0x01;
            else
                commonvalues::localdevstatus[channel].light = 0x00;
        }
    }

}

void MainWindow::sigCCUReceiveHandler(int channel, quint8 Cmd, QString data, QByteArray bdata)
{
    QString logstr;

    pmainthr->m_isignalflag |= mainthread::SIG_CCURcv;

    LogDlg *plogdlg;
    CCUComm *pccucomm;
    if(channel == 0)
    {
        plogdlg = plogdlg0;
        pccucomm = pccucomm0;

    }
    else
    {
        plogdlg = plogdlg1;
        pccucomm = pccucomm1;
    }


    if( Cmd == CCUComm::CMD_Connected)
    {
        logstr = QString("CCU Connected(ch%1)").arg(channel);
        plogdlg->logappend(LogDlg::loglic,logstr);

        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;

        quint8 seq = 0x00;
        quint8 becs = 0x01;
        quint16 imagenum = 0xFFFF;

        pccucomm->StatusREP_Send(seq, becs, imagenum);

    }
    else if(Cmd == CCUComm::CMD_Disconnected)
    {
        logstr = QString("CCU Disconnected(ch%1)").arg(channel);
        plogdlg->logappend(LogDlg::loglic,logstr);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
    }
    else if(Cmd == CCUComm::CMD_StatusREQ)
    {
        QStringList datalist = data.split(',');
        QString strseq = datalist[0];
        quint8 seq = (quint8)strseq.toUInt();
        quint8 becs = commonvalues::cameraSys[channel].old_becs;
        quint16 imagenum;
        if( commonvalues::cameraSys[channel].TGIndex == 1 )
             imagenum = 0xFFFF;
        else
             imagenum = commonvalues::cameraSys[channel].TGIndex - 1;

        pccucomm->StatusREP_Send(seq, becs, imagenum);

    }
    else if(Cmd == CCUComm::CMD_WORKSTART)
    {    //근무개시시간 //yyyyMMddHHmmss
        //원격서버에 WorkStart송신
        int client_count = commonvalues::clientlist.size();
        for(int index=0; index < client_count; index++)
        {
           if(commonvalues::clientlist[index]->protocol_type == CenterInfo::Remote)
           {
               commonvalues::clientlist[index]->m_workstartflag = 1;
           }
        }
////        QString strtime = data;
////        QDateTime dt = QDateTime::fromString(strtime,"yyyyMMddHHmmss");
////        QString timecmd = QString("timedatectl set-time '%1'").arg(dt.toString("yyyy-MM-dd HH:mm:ss"));
////        plog->write(QString("근무시간적용-%1").arg(timecmd),LOG_NOTICE);용
//        //system(timecmd.toStdString().c_str()); --> 에스트레픽에서는 근무개시 정보로 줌.동기화안맞음.
    }
    else if(Cmd == CCUComm::CMD_VIOLATION_REP_NEW)
    {
        QByteArray framedata = bdata;

        QString sTransNumber = QString::number((quint8)framedata[4]);
        quint16 uViolationNumber = (quint16)ArrtoUint(framedata.mid(5,2));
        QString sViolationTime = QString("%1%2%3%4%5%6")
                .arg((quint16)ArrtoUint(framedata.mid(7,2)),4,10,QChar('0')).arg((quint8)framedata[9],2,10,QChar('0')).arg((quint8)framedata[10],2,10,QChar('0'))
                .arg((quint8)framedata[11],2,10,QChar('0')).arg((quint8)framedata[12],2,10,QChar('0')).arg((quint8)framedata[13],2,10,QChar('0')); //위반일시 //yyyyMMddHHmmss
        QString sViolationType = QString::number(framedata[14]);
        quint8 WorkNumber1 = framedata[15];  //근무번호 - BCD(01~99), 차로
        quint8 WorkNumber2 = framedata[16]; //근무번호 - BCD, 근무인련번호
        QString sWorkDate = QString("%1%2%3")
                .arg((quint16)ArrtoUint(framedata.mid(17,2)),4,10,QChar('0')).arg((quint8)framedata[19],2,10,QChar('0')).arg((quint8)framedata[20],2,10,QChar('0'));  //근무일자 //yyyyMMdd
        quint32 uProcessNumber = ArrtoUint(framedata.mid(21,4));  // 처리번호
        QString sViolationCode = QString::number(framedata[25]); //위반코드
        QString sVehicleNumber = QString(framedata.mid(26,5).toHex());
        QString sWorkType = QString("%1").arg((quint8)framedata[31]);
        quint16 uProcessRegionNumber = (quint16)ArrtoUint(framedata.mid(32,2));
        QString sOBUNumber = QString(framedata.mid(34,8).toHex());
        quint8  OBUType = (quint8)framedata[42];
        quint8  etcnum = (quint8)framedata[43];
        quint8 bpay =  ( etcnum >> 4 ) & 0x0F;
        quint8 OBU_vehielcetype = etcnum & 0x0F;

        QString strCommand = QString("신위반확인응답 : %1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14,%15,%16")
                .arg(sTransNumber).arg(uViolationNumber).arg(sViolationTime).arg(sViolationType)
                .arg(WorkNumber1,2,16).arg(WorkNumber2,2,16).arg(sWorkDate).arg(uProcessNumber)
                .arg(sViolationCode).arg(sVehicleNumber).arg(sWorkType).arg(uProcessRegionNumber)
                .arg(sOBUNumber).arg(OBUType).arg(bpay).arg(OBU_vehielcetype);

        plogdlg->logappend(LogDlg::logccu,strCommand);
        plog->write(strCommand,LOG_NOTICE);

        if(uViolationNumber == 0) //아이트로닉스 쓰레기로버림
            return;

        //중복체크
        quint8 seq_check = (quint8)framedata[4]; //전송연번
        quint16 violationIndex_check = uViolationNumber;  //위반번호
        int ccucount = commonvalues::ccuViolationList[channel].count();
        if( ccucount > 0)
        {
            for(int cindex = 0;  cindex < ccucount; cindex++)
            {
                ViolationInfo violationinf = commonvalues::ccuViolationList[channel][cindex];
                if( seq_check == violationinf.seq && violationIndex_check == violationinf.violationIndex )
                {
                    logstr = QString("위반응답 중복(index:%1): seq-%2 , vioindex-%3").arg(ccucount)
                            .arg(seq_check).arg(violationIndex_check);
                    plog->write(logstr,LOG_INFO); qDebug() << logstr;
                    return;
                }
            }
        }

        ViolationInfo vioinfo;
        //vioinfo.rawdata = framedata.mid(3,VIOLATION_RAW_LEN); //Code(0x43) ~ 처리영업소번호까지 31bytes , 더미포함 - 41bytes
        memcpy(vioinfo.rawdata,framedata.data() + 3,sizeof(unsigned char) * VIOLATION_RAW_LEN);
        vioinfo.seq = (quint8)framedata[4];
        vioinfo.violationIndex = uViolationNumber;
        //vioinfo.violationTime = sViolationTime;
        strcpy((char*)vioinfo.violationTime,sViolationTime.toLocal8Bit().constData());
        vioinfo.violationCode = framedata[25];
        vioinfo.violationType = framedata[14];
        vioinfo.workNumber1 = WorkNumber1;
        vioinfo.workNumber2 = WorkNumber2;
       // vioinfo.workDate = sWorkDate;
        strcpy((char*)vioinfo.workDate,sWorkDate.toLocal8Bit().constData());
        vioinfo.processNumber = uProcessNumber;
        vioinfo.workType = framedata[31];
        vioinfo.exlocalNumber = uProcessRegionNumber;
        //vioinfo.obuNumber = sOBUNumber;
        strcpy((char*)vioinfo.obuNumber,sOBUNumber.toLocal8Bit().constData());

        QString viorecvTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
        strcpy((char*)vioinfo.recvTime,viorecvTime.toLocal8Bit().constData());
        //vioinfo.recvTime = QDateTime::currentDateTime().toString();

        commonvalues::ccuViolationList[channel].append(vioinfo);
        logstr = QString("위반응답 추가(index:%1): seq-%2 , vioindex-%3").arg(commonvalues::ccuViolationList[channel].count())
                .arg((quint8)vioinfo.seq).arg((quint16)vioinfo.violationIndex);
        plogdlg->logappend(LogDlg::logccu,logstr);
        plog->write(logstr,LOG_DEBUG); qDebug() << logstr;

        if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && commonvalues::cameraChannel > 1)
        {
            ViolationInfo vioinfo;
            //vioinfo.rawdata = framedata.mid(3,VIOLATION_RAW_LEN); //Code(0x43) ~ 처리영업소번호까지 31bytes  , 더미포함 - 41bytes
            memcpy(vioinfo.rawdata,framedata.data() + 3,sizeof(unsigned char) * VIOLATION_RAW_LEN);
            vioinfo.seq = framedata[4];
            vioinfo.violationIndex = uViolationNumber;
            //vioinfo.violationTime = sViolationTime;
            strcpy((char*)vioinfo.violationTime,sViolationTime.toLocal8Bit().constData());
            vioinfo.violationCode = framedata[25];
            vioinfo.violationType = framedata[14];
            vioinfo.workNumber1 = WorkNumber1;
            vioinfo.workNumber2 = WorkNumber2;
            //vioinfo.workDate = sWorkDate;
            strcpy((char*)vioinfo.workDate,sWorkDate.toLocal8Bit().constData());
            vioinfo.processNumber = uProcessNumber;
            vioinfo.workType = framedata[31];
            vioinfo.exlocalNumber = uProcessRegionNumber;
            commonvalues::ccuViolationList[1].append(vioinfo);
            logstr = QString("가상 위반응답 추가(index:%1)").arg(commonvalues::ccuViolationList[1].count());
            plogdlg->logappend(LogDlg::logccu,logstr);
            plog->write(logstr,LOG_DEBUG); qDebug() << logstr;
        }

        if( commonvalues::ccutype == commonvalues::CCU_ITRONICS )
        {
            if( commonvalues::cameraSys[channel].TGIndex != (int)uViolationNumber+1 )
            {
                if(uViolationNumber == 0xFFFF ) uViolationNumber = 0;  //영상번호 1~FFFF

                commonvalues::cameraSys[channel].TGIndex = (int)uViolationNumber+1;

                plogdlg->logappend(LogDlg::logccu,QString("CMD_VIOLATION_REP_NEW : SYNC(ch%1)-%2").arg(channel).arg(uViolationNumber));
                if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm )
                { //위반응답은 후방 트리거보다 늦게온다는 것을 전제함.
                    commonvalues::cameraSys[1].TGIndex = (int)uViolationNumber+1;
                }

    //            if(commonvalues::ccutype == commonvalues::CCU_ITRONICS)
    //            {
    //                pccucomm->VIOLATION_REQ_Send((quint8)framedata[4],uViolationNumber);
    //            }
            }
        }
        else if(  commonvalues::ccutype == commonvalues::CCU_DAEBO )
        {
            quint8 rseq;
            if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
            {
                if(pccucomm->m_sequenceindex == 0x01)
                    rseq = 0xFF;
                else
                    rseq = pccucomm->m_sequenceindex - 1;
            }
            else
            {
                if(pccucomm->m_sequenceindex == 0x01)
                    rseq = 0xFE;
                else if( pccucomm->m_sequenceindex == 0x02)
                    rseq = 0xFF;
                else
                    rseq = pccucomm->m_sequenceindex - 2;
            }

            if( commonvalues::cameraSys[channel].TGIndex != (int)uViolationNumber+1  && rseq == vioinfo.seq)
            {
                if(uViolationNumber == 0xFFFF ) uViolationNumber = 0;  //영상번호 1~FFFF

                commonvalues::cameraSys[channel].TGIndex = (int)uViolationNumber+1;

                plogdlg->logappend(LogDlg::logccu,QString("CMD_VIOLATION_REP_NEW : SYNC(ch%1)-%2").arg(channel).arg(uViolationNumber));
                if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm )
                {
                    commonvalues::cameraSys[1].TGIndex = (int)uViolationNumber+1;
                }

            }
        }
        else if( commonvalues::ccutype == commonvalues::CCU_sTRAFFIC )
        {
            quint8 rseq;
            if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
            {
                if(pccucomm->m_sequenceindex == 0x01)
                    rseq = 0xFF;
                else
                    rseq = pccucomm->m_sequenceindex - 1;
            }
            else
            {
                if(pccucomm->m_sequenceindex == 0x01)
                    rseq = 0xFE;
                else if( pccucomm->m_sequenceindex == 0x02)
                    rseq = 0xFF;
                else
                    rseq = pccucomm->m_sequenceindex - 2;
            }

            //근무개시 시 1에 대한 동기화,근무개시 시 연속차량에 대한 보완으로 3대까지를 염두에 둠.
            if( commonvalues::cameraSys[channel].TGIndex != (int)uViolationNumber+1 && uViolationNumber < 3  && rseq == vioinfo.seq)
            {
                commonvalues::cameraSys[channel].TGIndex = (int)uViolationNumber+1;

                plogdlg->logappend(LogDlg::logccu,QString("CMD_VIOLATION_REP_NEW : SYNC(ch%1)-%2").arg(channel).arg(uViolationNumber));
                if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm )
                {
                    commonvalues::cameraSys[1].TGIndex = (int)uViolationNumber+1;
                }

            }
            else if( commonvalues::cameraSys[channel].TGIndex != (int)uViolationNumber+1 && rseq == vioinfo.seq )
            { //부팅신호를 줘서 Sync신호를 유도함.
                pccucomm->m_bsyncREQsend = true;

                logstr = QString("Sync REQ(ch%1) : seq - %2").arg(channel).arg(vioinfo.seq);
                plogdlg->logappend(LogDlg::logccu,logstr);
                plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
            }

        }
        else if( commonvalues::ccutype == commonvalues::CCU_TCS_SDS )
        {
            if( commonvalues::cameraSys[channel].TGIndex != (int)uViolationNumber+1 )
            {
                if(uViolationNumber == 0xFFFF ) uViolationNumber = 0;  //영상번호 1~FFFF

                commonvalues::cameraSys[channel].TGIndex = (int)uViolationNumber+1;

                plogdlg->logappend(LogDlg::logccu,QString("CMD_VIOLATION_REP_NEW : SYNC(ch%1)-%2").arg(channel).arg(uViolationNumber));
                if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm )
                {
                    commonvalues::cameraSys[1].TGIndex = (int)uViolationNumber+1;
                }

            }
        }


    }
    else if( Cmd == CCUComm::CMD_CONFIRM_INFO)  //확정정보
    {
        QStringList datalist = data.split(',');
        if(datalist.count() < 7)
        {
            logstr = QString("확정정보 파싱에러(ch%1) : %2").arg(channel).arg(data);
            plogdlg->logappend(LogDlg::logccu,logstr);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
            return;
        }
        QString ProcessNumber = datalist[5];
        quint32 iProcessNumber = (quint32)ProcessNumber.toUInt();
        QString confirmlane = datalist[6];  // 0x01 : front , 0x02 : rear
        int iconfirmlane = confirmlane.toInt();
        logstr = QString("확정정보(ch%1) : %2/lane:%3").arg(channel).arg(iProcessNumber).arg(confirmlane);
        plogdlg->logappend(LogDlg::logccu,logstr);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
//        if(commonvalues::confirmSysType == commonvalues::CCU_Two_Confirm  && commonvalues::cameraChannel > 1)
//                             pmainthr->ConfirmInfo(channel,iProcessNumber,iconfirmlane);
//        else
//        {
            logstr = QString("No Use - CCU Confirm");
            plogdlg->logappend(LogDlg::logccu,logstr);
            plog->write(logstr,LOG_INFO); qDebug() << logstr;
//        }

    }
    else if( Cmd == CCUComm::CMD_VIOLATION_SYNC)
    {
        if(commonvalues::syncIgnore > 0)
        {
            logstr = QString("Sync Ignore(ch%1)").arg(channel);
            plogdlg->logappend(LogDlg::logccu,logstr);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
        }
        else
        {
            QStringList datalist = data.split(',');
            QString sTransNumber = datalist[0];
            quint8 iTransNumber =(quint8)sTransNumber.toUInt();
            QString sViolationNumber = datalist[1];
            quint16 uViolationNumber = (quint16)sViolationNumber.toUInt();

            quint16 tgindex = uViolationNumber;
            if(tgindex == 0xFFFF) tgindex = 1;
            else tgindex++;

            commonvalues::cameraSys[channel].TGIndex = tgindex;  //영상번호는1~FFFF
            plogdlg->logappend(LogDlg::logccu,QString("SYNC(ch%1): %2").arg(channel).arg(uViolationNumber));
            if(channel == 0 && commonvalues::confirmSysType == commonvalues::CCU_One_Confirm)
            {
                commonvalues::cameraSys[1].TGIndex = tgindex;
            }
        }

    }
    else
    {
        if(!data.isNull() && !data.isEmpty())
        {
            logstr = QString("LIC Cmd-%1, data-%2").arg(Cmd).arg(data);
            plog->write(logstr,LOG_INFO);
        }

    }
}

void MainWindow::sigTGMCUReceiveHandler(quint8 Cmd, QString data)
{
    QString logstr;

    pmainthr->m_isignalflag |= mainthread::SIG_TGMCURcv;

    if( Cmd == TGMCUComm::CMD_Connected)
    {
        logstr = QString("TGMCU Connected");
        if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::loglic,logstr);
        if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::loglic,logstr);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
    }
    else if(Cmd == TGMCUComm::CMD_Disconnected)
    {
        logstr = QString("TGMCU Disconnected");
        if(plogdlg0 != NULL)  plogdlg0->logappend(LogDlg::loglic,logstr);
        if(plogdlg1 != NULL)  plogdlg1->logappend(LogDlg::loglic,logstr);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
    }
    else if( Cmd == TGMCUComm::CMD_Trigger)
    {
        QString sLane = data;
        int ilane = sLane.toInt();

        int channel = 0;

        if(commonvalues::cameraSys[0].TGLane == ilane)
        {
            channel = 0;
            logstr = QString("Trigger Lane(%1) : %2(%3)").arg(channel).arg(ilane).arg(commonvalues::cameraSys[0].TGIndex);
            if(plogdlg0 != NULL)  plogdlg0->logappend(LogDlg::loglic,logstr);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
        }
        else if(commonvalues::cameraChannel > 1 && commonvalues::cameraSys[1].TGLane == ilane)
        {
            channel = 1;
            logstr = QString("Trigger Lane(ch%1) : %2").arg(channel).arg(ilane).arg(commonvalues::cameraSys[1].TGIndex);
            if(plogdlg1 != NULL)  plogdlg1->logappend(LogDlg::loglic,logstr);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
        }
        else
        {
//            logstr = QString("Error Trigger Lane : %1").arg(ilane);
//            if(plogdlg0 != NULL)  plogdlg0->logappend(LogDlg::loglic,logstr);
//            if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::loglic,logstr);
//            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
            return;
        }

        qint64 tgperiod = lastTGTime[channel].msecsTo(QDateTime::currentDateTime());
        if( tgperiod < 200 )
        {
            logstr = QString("Error Trigger Period(ch%1) : %2ms").arg(channel).arg(tgperiod);
            if(channel == 0 && plogdlg0 != NULL)  plogdlg0->logappend(LogDlg::loglic,logstr);
            if(channel == 1 && plogdlg1 != NULL) plogdlg1->logappend(LogDlg::loglic,logstr);
            plog->write(logstr,LOG_NOTICE); qDebug() << logstr;
            return;
        }
        lastTGTime[channel] = QDateTime::currentDateTime();

        if(commonvalues::cameraChannel > 1)
        {
            int ch0TG = commonvalues::cameraSys[0].TGIndex;
            int ch1TG = commonvalues::cameraSys[1].TGIndex;

            if( channel == 0)
            {
                if(ch0TG != ch1TG )
                {
                    if( ch0TG > ch1TG )
                            commonvalues::cameraSys[1].TGIndex = ch0TG;
                    else if( ch0TG < ch1TG)
                            commonvalues::cameraSys[0].TGIndex = ch1TG;

                    logstr = QString("Trigger MissMatching(%1/%2) -> (%3/%4)").arg(ch0TG).arg(ch1TG)
                            .arg(commonvalues::cameraSys[0].TGIndex)
                            .arg(commonvalues::cameraSys[1].TGIndex);
                    plog->write(logstr,LOG_NOTICE);
                    plogdlg0->logappend(LogDlg::loglic,logstr);
                }
            }
            else if( channel == 1 )
            {
                if(ch0TG != ( ch1TG + 1 ) )
                {
                    if( ch0TG > ch1TG )
                            commonvalues::cameraSys[1].TGIndex = ch0TG;
                    else if( ch0TG <= ch1TG)
                            commonvalues::cameraSys[0].TGIndex = ch1TG + 1;

                    logstr = QString("Trigger MissMatching(%1/%2) -> (%3/%4)").arg(ch0TG).arg(ch1TG)
                            .arg(commonvalues::cameraSys[0].TGIndex)
                            .arg(commonvalues::cameraSys[1].TGIndex);
                    plog->write(logstr,LOG_NOTICE);
                    plogdlg1->logappend(LogDlg::loglic,logstr);
                }
            }
        }


        spinview *pcamera;
        CCUComm *pccucomm;
        if(channel == 1 )
        {
            pcamera = pcamera1;
            pccucomm = pccucomm1;
        }
        else
        {
            pcamera = pcamera0;
            pccucomm = pccucomm0;
        }


        int index = commonvalues::veicheindex[channel] % MAX_VEHICLE;

        commonvalues::vehicledatalist[channel][index].vehicleproc = 0;
        commonvalues::vehicledatalist[channel][index].seq = commonvalues::veicheindex[channel];
        if(commonvalues::ccutype == commonvalues::CCU_ITRONICS )
        {
            commonvalues::vehicledatalist[channel][index].car_num = commonvalues::cameraSys[channel].TGIndex;//pccucomm->m_sequenceindex; //commonvalues::cameraSys[channel].TGIndex;
            commonvalues::vehicledatalist[channel][index].ccu_seq = pccucomm->m_sequenceindex;
            //영상동기화 CCU가 1개임.
            if(commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && channel == 0 && commonvalues::cameraChannel > 1)
                    pccucomm1->m_sequenceindex = pccucomm0->m_sequenceindex;
        }
        else if(commonvalues::ccutype == commonvalues::CCU_DAEBO )
        {
            commonvalues::vehicledatalist[channel][index].car_num = commonvalues::cameraSys[channel].TGIndex;//pccucomm->m_sequenceindex; //commonvalues::cameraSys[channel].TGIndex;
            commonvalues::vehicledatalist[channel][index].ccu_seq = pccucomm->m_sequenceindex;
            //영상동기화 CCU가 1개임.
            if(commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && channel == 0 && commonvalues::cameraChannel > 1)
                    pccucomm1->m_sequenceindex = pccucomm0->m_sequenceindex;
        }
        else if(commonvalues::ccutype == commonvalues::CCU_sTRAFFIC )
        {
            commonvalues::vehicledatalist[channel][index].car_num = commonvalues::cameraSys[channel].TGIndex;//pccucomm->m_sequenceindex; //commonvalues::cameraSys[channel].TGIndex;
            commonvalues::vehicledatalist[channel][index].ccu_seq = pccucomm->m_sequenceindex;
            //영상동기화 CCU가 1개임.
            if(commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && channel == 0 && commonvalues::cameraChannel > 1)
                    pccucomm1->m_sequenceindex = pccucomm0->m_sequenceindex;
        }
        else
        {   //Normal , TCS_SDS
            commonvalues::vehicledatalist[channel][index].car_num = commonvalues::cameraSys[channel].TGIndex;
            //영상동기화 CCU가 1개임.
            if(commonvalues::confirmSysType == commonvalues::CCU_One_Confirm && channel == 0 && commonvalues::cameraChannel > 1)
                    commonvalues::cameraSys[1].TGIndex = commonvalues::cameraSys[0].TGIndex;
        }
            //sdw 2017/05/07
        commonvalues::vehicledatalist[channel][index].recogresult = 0;
        strcpy((char *)commonvalues::vehicledatalist[channel][index].car_entrytime,QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss").toLocal8Bit().data());

        //command capture
        pcamera->procindex.append(index);
        commonvalues::vehicledatalist[channel][index].vehicleproc  |= commonvalues::spc_tg;
        commonvalues::vehicledatalist[channel][index].bVehicleNotification = false;
        //create vehicle db table column
        pdatabase->createdata(channel,index);
        if(commonvalues::cameraSys[channel].TGIndex == 0xFFFF ) commonvalues::cameraSys[channel].TGIndex = 0x0001;
        else commonvalues::cameraSys[channel].TGIndex++;

        //managed vehicle memory
        commonvalues::veicheindex[channel]++;
        if(commonvalues::veicheindex[channel] >= MAX_VEHICLEINDEX)
                commonvalues::veicheindex[channel] = 0;
    }
    else if( Cmd == TGMCUComm::CMD_SlotLaneREP)
    {

    }
}

void MainWindow::sigCameraStart(int channel)
{
    spinview *pcam;
    QLabel *lblModel;
    QLabel *lblResolution;
    QLabel *lblIP;
    QLabel *lblSN;

    if(channel == 1)
    {
        pcam = pcamera1;
        lblModel = ui->lblCameramodel1;
        lblResolution = ui->lblCameraResolution1;
        lblIP = ui->lblCameraIP1;
        lblSN = ui->lblCameraSN1;
    }
    else
    {
        pcam = pcamera0;
        lblModel = ui->lblCameramodel0;
        lblResolution = ui->lblCameraResolution0;
        lblIP = ui->lblCameraIP0;
        lblSN = ui->lblCameraSN0;
    }

    lblIP->setText(pcam->IPAddress);
    lblSN->setText(QString(pcam->pcamera->DeviceSerialNumber.GetValue()));
    lblModel->setText(QString(pcam->pcamera->DeviceModelName.GetValue()));
    lblResolution->setText(QString("%1 x %2").arg(pcam->image_width).arg(pcam->image_height));


}

void MainWindow::capture(int channel, int index, QImage *img)
{
    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "ImageCapture Enter" << index;

    //ui->lblCapImg->setScaledContents(true);
    if(channel == 0)
    {
        ui->lblCapImg0->setPixmap( QPixmap::fromImage(*img));
        ui->lblCapImg0->show();
    }
    else
    {
        ui->lblCapImg1->setPixmap( QPixmap::fromImage(*img));
        ui->lblCapImg1->show();
    }

    //qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "ImageCapture Leave" << index;
}

void MainWindow::recogcapture(int channel, int index)
{
    qDebug() << QDateTime::currentDateTime().toString("[hh:mm:ss:zzz] ") << "RecogCapture Enter" << commonvalues::vehicledatalist[channel][index].seq;

    //CCU 위반확인요구 전송
    CCUComm *pccucomm;
    RecognitionDlg *precognitiondlg;
    if(channel == 1)
    {
        pccucomm = pccucomm1;
        precognitiondlg = precognitiondlg1;
    }
    else
    {
        pccucomm = pccucomm0;
        precognitiondlg = precognitiondlg0;
    }

    //insert recog qlist
    VehcileIndex vindex;
    vindex.channel = channel;
    vindex.index = index;
    precogthr->procindex.append(vindex);
    //display recog dialog
    QImage displayimg = QImage(commonvalues::vehicledatalist[channel][index].img
                               ,commonvalues::cameraSys[channel].cam_image_width,commonvalues::cameraSys[channel].cam_image_height
                               ,QImage::Format_RGB888);

    precognitiondlg->display( &displayimg);


    pccucomm->VIOLATION_REQ_Send(pccucomm->m_sequenceindex,commonvalues::vehicledatalist[channel][index].car_num);
    if(commonvalues::seqincreaseType == commonvalues::One_Vehicle_One_Seq)
    {
        if(pccucomm->m_sequenceindex == 0xFF) pccucomm->m_sequenceindex = 0x01;
        else pccucomm->m_sequenceindex++;

    }
    else  //commonvalues::One_Vehicle_Two_Seq
    {
        if(pccucomm->m_sequenceindex == 0xFF)  pccucomm->m_sequenceindex = 0x02;
        else if(pccucomm->m_sequenceindex == 0xFE ) pccucomm->m_sequenceindex = 0x01;
        else pccucomm->m_sequenceindex += 2;
    }



}

void MainWindow::RecogEndSignal(int channel, int index)
{
    //insert mainthread qlist
    VehcileIndex vindex;
    vindex.channel = channel;
    vindex.index = index;
    qDebug() << QString("mainthr append VehicleIndex : %1, %2")
                .arg(channel).arg(index);
    pmainthr->procindex.append(vindex);

    //if(plogdlg != NULL) { plogdlg->logappend(logdlg::logrecog,"End Recog " + QString::number(commonvalues::vehicledatalist[index].seq)); }
    QLabel *lblPlateNumber = channel ==1 ? ui->lblPlateNumber1 : ui->lblPlateNumber0;
    QLabel *lblPlateImg = channel == 1  ? ui->lblPlateImg1 : ui->lblPlateImg0;
    autoiris *pautoiris = channel == 1 ? pautoiris1 : pautoiris0;

    //View PlateImage
    QString number = QString::fromLocal8Bit((const char*)commonvalues::vehicledatalist[channel][index].recognum);
    lblPlateNumber->setText(number);

    //if(number != "xxxxxx")
    if(commonvalues::vehicledatalist[channel][index].recogresult > 0 )
    {
        //QImage view1 = commonvalues::vehicledatalist[index].plateImage.copy();
        //ui->lblPlateImg->setScaledContents(true);
        lblPlateImg->setPixmap( QPixmap::fromImage(QImage(commonvalues::vehicledatalist[channel][index].plateImage,commonvalues::vehicledatalist[channel][index].plate_width
                                                              ,commonvalues::vehicledatalist[channel][index].plate_height,QImage::Format_RGB888)));
        lblPlateImg->show();
        //autoiris recog ok
        QByteArray img((char*)commonvalues::vehicledatalist[channel][index].plateImage,commonvalues::vehicledatalist[channel][index].plateImagelen);
        pautoiris->AutoIrisImageInsert(img,commonvalues::vehicledatalist[channel][index].plate_width,commonvalues::vehicledatalist[channel][index].plate_height,0);
    }
    else
    {
        //2016/09/12
        // no recog plate image clear
        lblPlateImg->clear();
        //autoiris no recog
        //QImage noview1 = commonvalues::vehicledatalist[index].img.copy();
        QByteArray img((char*)commonvalues::vehicledatalist[channel][index].img,commonvalues::vehicledatalist[channel][index].imglen);
        pautoiris->AutoIrisImageInsert(img,commonvalues::cameraSys[channel].cam_image_width,commonvalues::cameraSys[channel].cam_image_height,1);

    }
}

void MainWindow::SetLICStrobe(int channel,bool lightonoff)
{
    if(channel == 0)
        pliccomm0->SetLight(lightonoff);
    else if(channel == 1)
        pliccomm1->SetLight(lightonoff);
}

void MainWindow::LICTrigger(int channel)
{
    sigTGMCUReceiveHandler(TGMCUComm::CMD_Trigger, QString::number(commonvalues::cameraSys[channel].TGLane));
}

void MainWindow::recogdlgsaveslot(int channel)
{
//    camera->SetAutoExposureROI(commonvalues::recog_brroi_sx,commonvalues::recog_brroi_sy
    //                               ,commonvalues::recog_brroi_width, commonvalues::recog_brroi_height);
}

void MainWindow::createcenter(int index)
{
    //connect에서 센터이름을 적용할 경우 리붓시 일정시간동안 센터이름이 빈상태를 유지하게됨.
    //즉,FTP_Trans디렉토리에 영상이 저장됨.
    CenterClient *pctr = new CenterClient(index,commonvalues::center_list[index].centername,commonvalues::loglevel);

    connect(pctr, SIGNAL(CommandEvent(int,QString , QString)),this,SLOT(CenterCommandEvent(int, QString, QString)));
    if(plogdlg0 != NULL)
    {
        pctr->plogdlg0 = plogdlg0;
    }
    if(plogdlg1 != NULL)
    {
        pctr->plogdlg1 = plogdlg1;
    }

    commonvalues::clientlist.append(pctr);
    QString logstr = QString("Center Create : %1").arg(index);
    qDebug() << logstr;
}

void MainWindow::connectcenter(int index)
{
    CenterClient *pctr = commonvalues::clientlist[index];

    pctr->protocol_type = commonvalues::center_list[index].protocol_type;
    pctr->ConnectAsync(commonvalues::center_list[index].ip,QString::number(commonvalues::center_list[index].tcpport)
                                                 , commonvalues::center_list[index].centername,commonvalues::center_list[index].connectioninterval);

    QString logstr = QString("Center Connect(%1) : name-%2 ip-%3, port-%4, interval-%5")
            .arg(index).arg(commonvalues::center_list[index].centername).arg(commonvalues::center_list[index].ip)
            .arg(commonvalues::center_list[index].tcpport).arg(commonvalues::center_list[index].connectioninterval);
    if(plogdlg0 != NULL) plogdlg0->logappend(LogDlg::logcenter,logstr);
    if(plogdlg1 != NULL) plogdlg1->logappend(LogDlg::logcenter,logstr);
    plog->write(logstr,LOG_NOTICE); qDebug() << logstr;


}

void MainWindow::CenterCommandEvent(int id, QString cmd, QString data)
{
    QString logstr;
    //주제어기통신
    if( cmd.compare("T") == 0 )
    {
        if(id == 0)
        { // yyyyMMddHHmmss
             QDateTime dt = QDateTime::fromString(data,"yyyyMMddHHmmss");
             QString timecmd = QString("timedatectl set-time '%1'").arg(dt.toString("yyyy-MM-dd HH:mm:ss"));
             plog->write(QString("시간동기화-%1").arg(timecmd),LOG_NOTICE);
             system(timecmd.toStdString().c_str());

        }
    }
    //Remote 통신
    else if( cmd.compare("SYSTEM_REBOOT_REQ") == 0)
    {

        QStringList datalist = data.split(",");

        QString sReboot = datalist[2];
        int ireboot = sReboot.toInt();

        if( ireboot == 1)//warm boot
        {
            plog->write(QString("SYSTEM_REBOOT_REQ(%1) : Program Close").arg(id),LOG_ERR);
            QThread::msleep(200);
            this->close();
        }
        else if( ireboot == 2) //cold boot
        {
            plog->write(QString("SYSTEM_REBOOT_REQ(%1) : IPU Restart").arg(id),LOG_ERR);
            if(commonvalues::fw_update == commonvalues::FW_DOWNLOAD )
            {
                if(!commonvalues::fw_filename.isNull() && !commonvalues::fw_filename.isEmpty() )
                {
                    QString FTP_path =  QApplication::applicationDirPath() + "/Download/";
                    //QString strcmd = QString("dpkg --force-overwrite -i %1").arg(commonvalues::fw_filename);
                    QString strcmd = QString("dpkg --force-overwrite -i %1%2").arg(FTP_path).arg(commonvalues::fw_filename);

                    logstr = QString("Firmware Update : %1").arg(strcmd);
                    plog->write(logstr,LOG_NOTICE);

                    system(strcmd.toStdString().c_str());

                    commonvalues::fw_update = commonvalues::FW_UPDATE;
                    commonvalues::fw_resultTime = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
                    pcfg->setuint("FIRMWARE","Update",commonvalues::fw_update);
                    pcfg->set("FIRMWARE","ResultTime",commonvalues::fw_resultTime);
                    pcfg->save();
                    QThread::msleep(500);
                }
            }
            QThread::msleep(200);
            system("reboot -f");
        }

    }
    else if( cmd.compare("CAMERA_REBOOT_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        QString sCamChannel = datalist[2];
        int icamchannel = sCamChannel.toInt();

        if(icamchannel == 0)
            pliccomm0->Soft_Reset_Send(LICComm::RESET_CAMERA);
        else if(icamchannel == 1)
            pliccomm1->Soft_Reset_Send(LICComm::RESET_CAMERA);

    }
    else if( cmd.compare("ZOOM_CONTROL_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        QString sCamChannel = datalist[2];
        int icamchannel = sCamChannel.toInt();
        QString sControl = datalist[3];

        LICComm *pliccomm = NULL;
        if(icamchannel == 0)
        {
           pliccomm = pliccomm0;
        }
        else if(icamchannel == 1 )
        {
            pliccomm = pliccomm1;
        }
        else
            return;

        if(pliccomm == NULL )
            return;

        if(sControl.compare("+") == 0) //IN
        {
            quint8 type = 2;
            quint8 direction = 0;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
        else if(sControl.compare("-") == 0) //OUT
        {
            quint8 type = 2;
            quint8 direction = 1;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
    }
    else if( cmd.compare("FOCUS_CONTROL_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        QString sCamChannel = datalist[2];
        int icamchannel = sCamChannel.toInt();
        QString sControl = datalist[3];

        LICComm *pliccomm = NULL;
        if(icamchannel == 0)
        {
           pliccomm = pliccomm0;
        }
        else if(icamchannel == 1 )
        {
            pliccomm = pliccomm1;
        }
        else
            return;

        if(pliccomm == NULL )
            return;

        if(sControl.compare("+") == 0) //near
        {
            quint8 type = 3;
            quint8 direction = 0;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
        else if(sControl.compare("-") == 0) //far
        {
            quint8 type = 3;
            quint8 direction = 1;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
    }
    else if( cmd.compare("IRIS_CONTROL_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        QString sCamChannel = datalist[2];
        int icamchannel = sCamChannel.toInt();
        QString sControl = datalist[3];

        LICComm *pliccomm = NULL;
        if(icamchannel == 0)
        {
           pliccomm = pliccomm0;
        }
        else if(icamchannel == 1 )
        {
            pliccomm = pliccomm1;
        }
        else
            return;

        if(pliccomm == NULL )
            return;

        if(sControl.compare("+") == 0) //open
        {
            quint8 type = 4;
            quint8 direction = 0;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
        else if(sControl.compare("-") == 0) //close
        {
            quint8 type = 4;
            quint8 direction = 1;
            quint16 imove = LENS_CONTROL_TIME;
            pliccomm->Time_control_Send(type,direction,imove);
        }
    }
    else if( cmd.compare("FTPCreate") == 0)
    {
        if( commonvalues::clientlist[id]->m_pftp == NULL )
        {
          commonvalues::clientlist[id]->m_pftp = new QFtp(this);
          logstr = QString("FTPCreate(ch%1) OK").arg(id);
          plog->write(logstr,LOG_NOTICE);  qDebug() << logstr;
        }
        else
        {
            logstr = QString("FTPCrvveate(ch%1) Fail( ftp isn't NULL )").arg(id);
            plog->write(logstr,LOG_NOTICE);  qDebug() << logstr;
        }
    }
    else if( cmd.compare("FW_UPDATE_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        if(datalist.size() < 2)
        {
            return;
        }

        logstr = QString("FW_UPDATE_REQ Event: %1, %2").arg(datalist[0]).arg(datalist[1]);
        plog->write(logstr,LOG_NOTICE); qDebug() << logstr;

        commonvalues::fw_update = commonvalues::FW_DOWNLOAD;
        commonvalues::fw_controlNo = datalist[0];
        commonvalues::fw_filename = datalist[1];

        //pcfg->setuint("FIRMWARE","Update",commonvalues::fw_update);
        pcfg->set("FIRMWARE","ControlNo",commonvalues::fw_controlNo);
        pcfg->set("FIRMWARE","FileName",commonvalues::fw_filename);

    }
    else if( cmd.compare("FW_UPDATE_RESULT") == 0 )
    {
        commonvalues::fw_update = commonvalues::FW_NONE;
        pcfg->setuint("FIRMWARE","Update",commonvalues::fw_update);
        pcfg->save();
    }
    else if( cmd.compare("OBU_MACH_UPDATE_REQ") == 0)
    {
        QStringList datalist = data.split(",");

        if(datalist.size() < 2)
        {
            return;
        }

        QString strControlNo(datalist[0].data(),datalist[0].size());
        QString strOBUFile(datalist[1].data(),datalist[1].size());
        if(!pdatabase->SetOBUDB(true,id,strControlNo,strOBUFile))
        {
            logstr = QString("----- SetOBUDB Failed(%1)").arg(strOBUFile);
            plog->write(logstr,LOG_NOTICE);  qDebug() << logstr;

            QString returndata2 = QString("%1,%2,%3,%4")
                    .arg(strControlNo).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
                    .arg("미적용").arg(strOBUFile);
            commonvalues::clientlist[id]->CommandRESP("CONTROL","OBU_MACH_UPDATE_RESULT",returndata2);
        }

    }

}

void MainWindow::OBUUpdateResult(int id, QString sControlNo, bool bupdate, QString sFilename)
{
    int count = commonvalues::clientlist.count();
    if( id >= count )
            return;

    QString returndata2 = QString("%1,%2,%3,%4")
            .arg(sControlNo).arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"))
            .arg(bupdate ? "적용" : "미적용").arg(sFilename);
    commonvalues::clientlist[id]->CommandRESP("CONTROL","OBU_MACH_UPDATE_RESULT",returndata2);
}

void MainWindow::closeEvent(QCloseEvent *)
{
    int close_count;
    //dialog
   if(pcameraconfigdlg0 != NULL)pcameraconfigdlg0->close();
   if(pcameraconfigdlg1 != NULL)pcameraconfigdlg1->close();
   if(pcontrolboarddlg0 != NULL)pcontrolboarddlg0->close();
   if(pcontrolboarddlg1 != NULL)pcontrolboarddlg1->close();

   if(papsgdlg0 != NULL)papsgdlg0->close();
   if(papsgdlg1 != NULL)papsgdlg1->close();
   if(precognitiondlg0 != NULL)precognitiondlg0->close();
   if(precognitiondlg1 != NULL)precognitiondlg1->close();

   if(plogdlg0 != NULL)plogdlg0->close();
   if(plogdlg1 != NULL)plogdlg1->close();
   if(pconfigdlg != NULL)pconfigdlg->close();
   if(pcenterdlg != NULL)pcenterdlg->close();
   if(pdatabasedlg != NULL)pdatabasedlg->close();

    qTimer->stop();
    qTimer->deleteLater();

    if( ptgmcucomm != NULL)
    {
        ptgmcucomm->stop();
        close_count = 0;
        while(true)
        {
            if(ptgmcucomm->isFinished()) break;
            close_count++;
            QThread::msleep(1);
            if(close_count > 1000)
                break;
        }
    }


    for(int ch =0; ch < 2; ch++)
    {
        if(ch == 0)
        {
            if(pautoiris0 != NULL)
            {
                pautoiris0->stop();
                close_count = 0;
                while(true)
                {
                    if(pautoiris0->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }

            }
            if(pliccomm0 != NULL)
            {
                pliccomm0->stop();
                close_count = 0;
                while(true)
                {
                    if(pliccomm0->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }
            if(pccucomm0 != NULL)
            {
                pccucomm0->stop();
                close_count = 0;
                while(true)
                {
                    if(pccucomm0->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }
            if(pcamera0 != NULL)
            {
                pcamera0->stop();
                close_count = 0;
                while(true)
                {
                    if(pcamera0->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }

        }
        if( ch == 1)
        {
            if(pautoiris1 != NULL)
            {
                pautoiris1->stop();
                close_count = 0;
                while(true)
                {
                    if(pautoiris1->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }

            }
            if(pliccomm1 != NULL)
            {
                pliccomm1->stop();
                close_count = 0;
                while(true)
                {
                    if(pliccomm1->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }
            if(pccucomm1 != NULL)
            {
                pccucomm1->stop();
                close_count = 0;
                while(true)
                {
                    if(pccucomm1->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }
            if(pcamera1 != NULL)
            {
                pcamera1->stop();
                close_count = 0;
                while(true)
                {
                    if(pcamera1->isFinished()) break;
                    close_count++;
                    QThread::msleep(1);
                    if(close_count > 1000)
                        break;
                }
            }
        }

    }
    if( precogthr != NULL)
    {
        precogthr->stop();
        close_count = 0;
        while(true)
        {
            if(precogthr->isFinished()) break;
            close_count++;
            QThread::msleep(1);
            if(close_count > 1000)
                break;
        }
    }

    int centercount = commonvalues::clientlist.size();
    for(int i=0;i < centercount; i++)
    {
        commonvalues::clientlist.value(i)->Disconnect();
    }
    for(int i=0;i < centercount; i++)
    {
        commonvalues::clientlist.value(i)->stop();
        close_count = 0;
        while(true)
        {
            if(commonvalues::clientlist.value(i)->isFinished()) break;
            close_count++;
            QThread::msleep(1);
            if(close_count > 1000)
                break;
        }
        delete commonvalues::clientlist.value(i);
    }

    if( pmainthr != NULL)
    {
        pmainthr->stop();
        close_count = 0;
        while(true)
        {
            if(pmainthr->isFinished()) break;
            close_count++;
            QThread::msleep(1);
            if(close_count > 1000)
                break;
        }

    }
    if(pdatabase != NULL )
    {
        pdatabase->stop();
        close_count = 0;
        while(true)
        {
            if(pdatabase->isFinished()) break;
            close_count++;
            QThread::msleep(1);
            if(close_count > 1000)
                break;
        }
    }
    // vehicle memory free
    free_vehiclemem();

}

void MainWindow::onTimer()
{
    m_mseccount++;

    if( m_mseccount%2 == 0)
            CurrentValueStatus();

    if( m_mseccount%3 == 0) // 3ÃÊ ÁÖ±â
    {        
        DeviceStatus();
    }

    if( m_mseccount%5 == 0)
    {
        //status bar -> display center connection status
        checkcenterstatus();
    }

    if( m_mseccount%30 == 0 )
    {
        for(int index=0; index < commonvalues::cameraChannel; index++)
        {
            if(index == 0)
                if(pliccomm0 != NULL) pliccomm0->StatusREQ_Send();
            else if( index == 1)
                if(pliccomm1 != NULL) pliccomm1->StatusREQ_Send();
        }
    }


    if( bclose )
    {
        this->close();
    }
    if( m_mseccount%60 == 0) //1분단위
    {
        if(commonvalues::daychange > 0)
        {
            qint64 days = bootingTime.daysTo(QDateTime::currentDateTime());
            if( days > 0 )
            {
                QString logstr = QString("Day change : %1 -> %2")
                        .arg(bootingTime.toString("yyyyMMddHHmmss"))
                        .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));
                plog->write(logstr,LOG_NOTICE);
                bclose = true;
            }
        }
        else
        {
            bootingTime = QDateTime::currentDateTime();
        }

        //SerialThread Error => Program Close
        if(!SerialThreadCheck())
        {
            bclose = true;
        }
    }

    pmainthr->m_isignalflag |= mainthread::SIG_OnTimer;

}

void MainWindow::CurrentValueStatus()
{
    QLabel *lblBr;
    QLabel *lblCDS;
    QLabel *lblShutter;
    QLabel *lblGain;
    QLabel *lblLight;
    QLabel *lblTemp;
    autoiris *pautoiris;

    QLabel *lblFPS;

    for(int ch=0; ch < commonvalues::cameraChannel; ch++)
    {
        if( ch == 0)
        {
            lblBr = ui->lblBr0;
            lblCDS = ui->lblCDS0;
            lblShutter = ui->lblShutter0;
            lblGain = ui->lblGain0;
            lblLight = ui->lblLight0;
            lblTemp = ui->lblTemperature0;
            pautoiris = pautoiris0;

            lblFPS = ui->lblCameraFramerate0;
        }
        else if( ch == 1 )
        {
            lblBr = ui->lblBr1;
            lblCDS = ui->lblCDS1;
            lblShutter = ui->lblShutter1;
            lblGain = ui->lblGain1;
            lblLight = ui->lblLight1;
            lblTemp = ui->lblTemperature1;
            pautoiris = pautoiris1;

            lblFPS = ui->lblCameraFramerate1;
        }
        CurrentValue curtvalue = commonvalues::currentvalue[ch];

        lblBr->setText(QString("%1/(%2)").arg(curtvalue.cur_br).arg(pautoiris->gi_TargetBr));
        lblCDS->setText(QString::number(curtvalue.cur_cds));
        lblShutter->setText(QString::number(curtvalue.cur_shutter,'f',2));
        lblGain->setText(QString::number(curtvalue.cur_gain,'f',2));
        lblLight->setText(curtvalue.cur_light ? "ON" : "OFF");
        lblTemp->setText(QString::number(curtvalue.cur_temp));

        lblFPS->setText(QString::number(curtvalue.cur_fps));

    }

}

void MainWindow::DeviceStatus()
{
    //display status
    //camera status check & restart
    for(int ch=0 ; ch < commonvalues::cameraChannel; ch++)
    {

        spinview *pcamera;
        LICComm *plic;
        CCUComm *pccu;
        TGMCUComm *ptgmcu;
        dbmng *pdb;

        QLabel *stCam;
        QLabel *stLIC;
        QLabel *stCCU;
        QLabel *stTG;
        QLabel *stDB;

        if(ch == 1)
        {
            pcamera = pcamera1;
            plic = pliccomm1;
            pccu = pccucomm1;
            ptgmcu = ptgmcucomm;
            pdb = pdatabase;

            stCam = ui->lblCameraStatus1;
            stLIC = ui->lblLICStatus1;
            stCCU = ui->lblCCUStatus1;
            stTG = ui->lblTGMCUStatus1;
            stDB = ui->lblDBStatus1;
        }
        else
        {
            pcamera = pcamera0;
            plic = pliccomm0;
            pccu = pccucomm0;
            ptgmcu = ptgmcucomm;
            pdb = pdatabase;

            stCam = ui->lblCameraStatus0;
            stLIC = ui->lblLICStatus0;
            stCCU = ui->lblCCUStatus0;
            stTG = ui->lblTGMCUStatus0;
            stDB = ui->lblDBStatus0;
        }

        if(pcamera->m_state > 0 )
        {
            stCam->setStyleSheet("QLabel { background-color : red; }");
            //camera restart
            if(m_restartcount[ch] < 0)
            {
                emit camera_restart(ch);
                //ai->m_bcamerarestart = true;
                m_restartcount[ch] = 2;
            }
            else m_restartcount[ch]--;
        }
        else stCam->setStyleSheet("QLabel { background-color : green; }");

        if(plic->m_state > 0 ) stLIC->setStyleSheet("QLabel { background-color : red; }");
        else stLIC->setStyleSheet("QLabel { background-color : green; }");

        if(pccu->m_state > 0 ) stCCU->setStyleSheet("QLabel { background-color : red; }");
        else stCCU->setStyleSheet("QLabel { background-color : green; }");

        if(ptgmcu->m_state > 0 ) stTG->setStyleSheet("QLabel { background-color : red; }");
        else stTG->setStyleSheet("QLabel { background-color : green; }");

        if(pdb->m_state > 0 ) stDB->setStyleSheet("QLabel { background-color : red; }");
        else stDB->setStyleSheet("QLabel { background-color : green; }");
   }
}


//login setting
// login time -> default : 600sec
void MainWindow::loginsetting(bool loginstate, int logintime)
{
    m_blogin = loginstate;
    m_logintime = logintime;

    //show/hidden
    if( m_blogin ) //login ok
    {
        ui->btnLogin->setText("Log Out");
    }
    else
    {
        ui->btnLogin->setText("Log In");
        //display checkalbe
        ui->chkDisplay0->setChecked(m_blogin);
        //hidd
        on_chkDisplay0_clicked();
        //action dialog hidden
        for(int ch=0; ch < commonvalues::cameraChannel; ch++)
        {
            if(ch == 0)
            {
                pcameraconfigdlg0->hide();
                pcontrolboarddlg0->hide();
                papsgdlg0->hide();
                plogdlg0->hide();
            }
            else if(ch == 1)
            {
                pcameraconfigdlg1->hide();
                pcontrolboarddlg1->hide();
                papsgdlg1->hide();
                plogdlg1->hide();
            }
        }
        pcenterdlg->hide();        
        pconfigdlg->hide();
    }


    for(int ch=0; ch < commonvalues::cameraChannel; ch++)
    {

        if(ch == 0)
        {
            ui->menuChannel0->setEnabled(m_blogin);
            ui->actionCameraConfig0->setEnabled(m_blogin);
            ui->actionAPSG0->setEnabled(m_blogin);
            ui->actionControlBoard0->setEnabled(m_blogin);
            ui->actionLog0->setEnabled(m_blogin);
            ui->chkDisplay0->setEnabled(m_blogin);
        }
        else if(ch == 1)
        {
             ui->menuChannel1->setEnabled(m_blogin);
             ui->actionCameraConfig1->setEnabled(m_blogin);
             ui->actionAPSG1->setEnabled(m_blogin);
             ui->actionControlBoard1->setEnabled(m_blogin);
             ui->actionLog1->setEnabled(m_blogin);
             ui->chkDisplay1->setEnabled(m_blogin);
        }

    }
    //log level
    ui->cbLogLevel->setEnabled(m_blogin);
    ui->btnLogLevelSet->setEnabled(m_blogin);
    //repeat test
    ui->leRepeatShotInterval->setEnabled(m_blogin);
    ui->btnRepeatShot->setEnabled(m_blogin);
    //menu setting
    ui->menuCommon->setEnabled(m_blogin);
    //action
    ui->actionConfiguration->setEnabled(m_blogin);
    ui->actionDB->setEnabled(m_blogin);
    ui->actionCenter->setEnabled(m_blogin);
    //display enable
}

quint32 MainWindow::ArrtoUint(QByteArray bdata)
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

bool MainWindow::SerialThreadCheck()
{
   QString logstr;
   bool brtn = true;

   if( pliccomm0->m_loopcount == 0 )
   {
       logstr = QString("Serial Loop hanging(LIC)");
       plog->write(logstr,LOG_ERR);
       brtn = false;
   }
   else
       pliccomm0->m_loopcount = 0;

   if( pccucomm0->m_loopcount == 0)
   {
       logstr = QString("Serial Loop hanging(CCU)");
       plog->write(logstr,LOG_ERR);
       brtn = false;
   }
   else
       pccucomm0->m_loopcount=0;

   if( ptgmcucomm->m_loopcount == 0 )
   {
       logstr = QString("Serial Loop hanging(TGMCU)");
       plog->write(logstr,LOG_ERR);
        brtn = false;
   }
   else
       ptgmcucomm->m_loopcount = 0;


   //ccu bcc error count check
   if( pccucomm0->m_bccerrorcount > MAXBCCERRORCOUNT)
   {
       logstr = QString("CCU(ch0) BCC ERROR COUNT OVER %1").arg(MAXBCCERRORCOUNT);
       plog->write(logstr,LOG_ERR);
       brtn = false;
   }

   return brtn;
}

void MainWindow::on_btnLogin_clicked()
{
    if(!m_blogin)
    {
        QString login_id = ui->leLoginID->text();
        QString login_pw = ui->leLoginPW->text();

        if( login_id == "inpeg" && login_pw == "inpeg1111")
        {
            loginsetting(true);
        }
        else loginsetting(false);
    }
    else
    {
        loginsetting(false);
    }
}

void MainWindow::on_btnLogLevelSet_clicked()
{
    commonvalues::loglevel = ui->cbLogLevel->currentIndex();
    pcfg->setuint("LOG","Level",commonvalues::loglevel);
    pcfg->save();

    loglevelsetting(commonvalues::loglevel);

    plog->write(QString("loglevel setting : %1").arg(commonvalues::loglevel),LOG_NOTICE);
}

void MainWindow::on_chkDisplay0_clicked()
{
    pcamera0->bdisplay = ui->chkDisplay0->isChecked();
}

void MainWindow::on_chkDisplay1_clicked()
{
     pcamera1->bdisplay = ui->chkDisplay1->isChecked();
}

void MainWindow::on_btnCCUReconnection_clicked()
{
    pccucomm0->SerialDeviceSetting();
    //pccucomm0->handleError(QSerialPort::SerialPortError::ResourceError);
}
