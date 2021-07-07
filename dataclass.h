#ifndef DATACLASS_H
#define DATACLASS_H

#include <QObject>
#include <QLabel>


class clsHipassCarData
{
public:
    clsHipassCarData()
    {
        TRIGGER_DATETIME = "";
        TRIGGER_NUMBER = "0";
        TRIGGER_LANE = "1";
        TRIGGER_POSION = "전면";
        SELECT_DATA = "확정";
        PROCESS_RESULT = "정상";
        PROCESS_NUMBER = "0";
        PROCESS_WAY = "동영상";
        WORK_NO = "";
        WORK_DATE = "";
        IMAGE_NUMBER = "";
        VIO_DATETIME = "";
        VIO_TYPE = "";
        VIO_CODE = "";
        RECOG_RESULT = "";
        RECOG_CAR_NO = "";
        RECOG_CHAR_SCORE = "";
        CHAR_MACHING_YN = "";
        CURR_STROBE_STATE = "켜짐";
        CURR_ZOOM_VALUE = "";
        CURR_FOCUS_VALUE = "";
        CURR_IRIS_VALUE = "";
        CURR_CDS_VALUE = "";
        IMAGE_FILE_NAME = "";
        PROCESS_ALGORITHM = "0";
    }
    clsHipassCarData(clsHipassCarData *data)
    {
        TRIGGER_DATETIME = QString(data->TRIGGER_DATETIME.constData(),data->TRIGGER_DATETIME.size());
        TRIGGER_NUMBER = QString(data->TRIGGER_NUMBER.constData(),data->TRIGGER_NUMBER.size());
        TRIGGER_LANE = QString(data->TRIGGER_LANE.constData(),data->TRIGGER_LANE.size());
        TRIGGER_POSION = QString(data->TRIGGER_POSION.constData(),data->TRIGGER_POSION.size());;
        SELECT_DATA = QString(data->SELECT_DATA.constData(),data->SELECT_DATA.size());
        PROCESS_RESULT = QString(data->PROCESS_RESULT.constData(),data->PROCESS_RESULT.size());
        PROCESS_NUMBER = QString(data->PROCESS_NUMBER.constData(),data->PROCESS_NUMBER.size());
        PROCESS_WAY = QString(data->PROCESS_WAY.constData(),data->PROCESS_WAY.size());
        WORK_NO = QString(data->WORK_NO.constData(),data->WORK_NO.size());
        WORK_DATE = QString(data->WORK_DATE.constData(),data->WORK_DATE.size());
        IMAGE_NUMBER = QString(data->IMAGE_NUMBER.constData(),data->IMAGE_NUMBER.size());
        VIO_DATETIME = QString(data->VIO_DATETIME.constData(),data->VIO_DATETIME.size());
        VIO_TYPE = QString(data->VIO_TYPE.constData(),data->VIO_TYPE.size());
        VIO_CODE = QString(data->VIO_CODE.constData(),data->VIO_CODE.size());
        RECOG_RESULT = QString(data->RECOG_RESULT.constData(),data->RECOG_RESULT.size());
        RECOG_CAR_NO = QString(data->RECOG_CAR_NO.constData(),data->RECOG_CAR_NO.size());
        RECOG_CHAR_SCORE = QString(data->RECOG_CHAR_SCORE.constData(),data->RECOG_CHAR_SCORE.size());
        CHAR_MACHING_YN = QString(data->CHAR_MACHING_YN.constData(),data->CHAR_MACHING_YN.size());
        CURR_STROBE_STATE = QString(data->CURR_STROBE_STATE.constData(),data->CURR_STROBE_STATE.size());
        CURR_ZOOM_VALUE = QString(data->CURR_ZOOM_VALUE.constData(),data->CURR_ZOOM_VALUE.size());
        CURR_FOCUS_VALUE = QString(data->CURR_FOCUS_VALUE.constData(),data->CURR_FOCUS_VALUE.size());
        CURR_IRIS_VALUE = QString(data->CURR_IRIS_VALUE.constData(),data->CURR_IRIS_VALUE.size());
        CURR_CDS_VALUE = QString(data->CURR_CDS_VALUE.constData(),data->CURR_CDS_VALUE.size());
        IMAGE_FILE_NAME = QString(data->IMAGE_FILE_NAME.constData(),data->IMAGE_FILE_NAME.size());
        PROCESS_ALGORITHM = QString(data->PROCESS_ALGORITHM.constData(),data->PROCESS_ALGORITHM.size());
    }

    //대입연산자
    clsHipassCarData &operator= (const clsHipassCarData &other)
    {
        TRIGGER_DATETIME = QString(other.TRIGGER_DATETIME.constData(),other.TRIGGER_DATETIME.size());
        TRIGGER_NUMBER = QString(other.TRIGGER_NUMBER.constData(),other.TRIGGER_NUMBER.size());
        TRIGGER_LANE = QString(other.TRIGGER_LANE.constData(),other.TRIGGER_LANE.size());
        TRIGGER_POSION = QString(other.TRIGGER_POSION.constData(),other.TRIGGER_POSION.size());;
        SELECT_DATA = QString(other.SELECT_DATA.constData(),other.SELECT_DATA.size());
        PROCESS_RESULT = QString(other.PROCESS_RESULT.constData(),other.PROCESS_RESULT.size());
        PROCESS_NUMBER = QString(other.PROCESS_NUMBER.constData(),other.PROCESS_NUMBER.size());
        PROCESS_WAY = QString(other.PROCESS_WAY.constData(),other.PROCESS_WAY.size());
        WORK_NO = QString(other.WORK_NO.constData(),other.WORK_NO.size());
        WORK_DATE = QString(other.WORK_DATE.constData(),other.WORK_DATE.size());
        IMAGE_NUMBER = QString(other.IMAGE_NUMBER.constData(),other.IMAGE_NUMBER.size());
        VIO_DATETIME = QString(other.VIO_DATETIME.constData(),other.VIO_DATETIME.size());
        VIO_TYPE = QString(other.VIO_TYPE.constData(),other.VIO_TYPE.size());
        VIO_CODE = QString(other.VIO_CODE.constData(),other.VIO_CODE.size());
        RECOG_RESULT = QString(other.RECOG_RESULT.constData(),other.RECOG_RESULT.size());
        RECOG_CAR_NO = QString(other.RECOG_CAR_NO.constData(),other.RECOG_CAR_NO.size());
        RECOG_CHAR_SCORE = QString(other.RECOG_CHAR_SCORE.constData(),other.RECOG_CHAR_SCORE.size());
        CHAR_MACHING_YN = QString(other.CHAR_MACHING_YN.constData(),other.CHAR_MACHING_YN.size());
        CURR_STROBE_STATE = QString(other.CURR_STROBE_STATE.constData(),other.CURR_STROBE_STATE.size());
        CURR_ZOOM_VALUE = QString(other.CURR_ZOOM_VALUE.constData(),other.CURR_ZOOM_VALUE.size());
        CURR_FOCUS_VALUE = QString(other.CURR_FOCUS_VALUE.constData(),other.CURR_FOCUS_VALUE.size());
        CURR_IRIS_VALUE = QString(other.CURR_IRIS_VALUE.constData(),other.CURR_IRIS_VALUE.size());
        CURR_CDS_VALUE = QString(other.CURR_CDS_VALUE.constData(),other.CURR_CDS_VALUE.size());
        IMAGE_FILE_NAME = QString(other.IMAGE_FILE_NAME.constData(),other.IMAGE_FILE_NAME.size());
        PROCESS_ALGORITHM = QString(other.PROCESS_ALGORITHM.constData(),other.PROCESS_ALGORITHM.size());

       return *this;
    }

public:
    QString TRIGGER_DATETIME;
    QString TRIGGER_NUMBER;
    QString TRIGGER_LANE;
    QString TRIGGER_POSION;
    QString SELECT_DATA;
    QString PROCESS_RESULT;
    QString PROCESS_NUMBER;
    QString PROCESS_WAY;
    QString WORK_NO;
    QString WORK_DATE;
    QString IMAGE_NUMBER;
    QString VIO_DATETIME;
    QString VIO_TYPE;
    QString VIO_CODE;
    QString RECOG_RESULT;
    QString RECOG_CAR_NO;
    QString RECOG_CHAR_SCORE;
    QString CHAR_MACHING_YN;
    QString CURR_STROBE_STATE;
    QString CURR_ZOOM_VALUE;
    QString CURR_FOCUS_VALUE;
    QString CURR_IRIS_VALUE;
    QString CURR_CDS_VALUE;
    QString IMAGE_FILE_NAME;
    QString PROCESS_ALGORITHM;

};


class clsCameraSTATUS
{
public:
    clsCameraSTATUS()
    {
        CAM_NO = "00";
        LANE_INDEX = "1";
        CAM_POSTION = "전면";
        IMAGE_RESOLUTION = "1920x1080";
        ZOOMCONTROL_MINRANGE = "10";
        ZOOMCONTROL_MAXRANGE = "1023";
        FOCUSCONTROL_MINRANGE = "10";
        FOCUSCONTROL_MAXRANGE = "1023";
        IRISCONTROL_MINRANGE = "10";
        IRISCONTROL_MAXRANGE = "1023";
        CAMERA_STATE = "정상";
        LENSCONTROLBOARD_STATE = "정상";
        DETECTOR_STATE = "정상";
        STROBE_STATE = "켜짐";
    }

public:
    QString CAM_NO;
    QString LANE_INDEX;
    QString CAM_POSTION;
    QString IMAGE_RESOLUTION;
    QString ZOOMCONTROL_MINRANGE;
    QString ZOOMCONTROL_MAXRANGE;
    QString FOCUSCONTROL_MINRANGE;
    QString FOCUSCONTROL_MAXRANGE;
    QString IRISCONTROL_MINRANGE;
    QString IRISCONTROL_MAXRANGE;
    QString CAMERA_STATE;
    QString LENSCONTROLBOARD_STATE;
    QString DETECTOR_STATE;
    QString STROBE_STATE;
};

class clsRemoteDATA
{
public:
    clsRemoteDATA(int camerachannel)
    {
        FILE_SIZE = "";  //acsii  5자리
        DATA_TYPE = "COLLECT"; //COLLECT/CONTROL
        SYS_TYPE = "IMAGE";
        SEND_TIME = "";  //YYYY MM DD HH MM SS MSE 17자리
        IC_CODE = "0000";    //영업소번호 4자리
        LANE_NO = "00";    //차로번호 2자리
        BD_NAME = "IPUH_F10";    //보드명
        MSG_TYPE = "";   //메시지종류
        MAKER_NAME = "JWIN"; //제조사영문  진우(JWIN)
        INTERFACE_VERSION = "1.1";  //인터페이스버전,제조사마다 다를 수 있음 3자리
        FW_H_VERSION = "";

        //근무개시 데이터
        WORK_START_DATETIME = "";
        LANE_TYPE = "";
        LANE_SYS_TYPE = "1";

        //차로상태 데이터
        LANE_KIND = "0";
        MANUFACTURER = "진우산전";
        MANU_YEAR = "201912";
        FW_VERSION = "";
        FW_STATE = "";
        SYS_FORM_TYPE = "";
        CAMERA_CNT = QString("%1").arg(camerachannel,0,2,QChar('0'));
        CONTROLLERCONNECTION_STATE = "";
        FTPCONNECTION_STATE = "";
        //QString CAMERA_CNT  : 카메라개수
        //CAM_INFO  - 카메라정보

        for(int index=0 ; index < camerachannel; index++)
        {
            clsCameraSTATUS camStatus;
            camStatus.CAM_NO = QString("%1").arg(index,2,10,QChar('0'));
            if(index > 0 )
                camStatus.CAM_POSTION = "후면";
            cameraST.append(camStatus);
        }
        //보수데이터
        DEVICE_NAME = "";
        DEVICE_INDEX = "";
        REPAIR_EVENT = "";

        //펌웨어정보조회응답
        FW_DIV = "";
        FW_VER = "1.0.0";
    }


public:
    //공통
    QString FILE_SIZE;  //acsii  5자리
    QString DATA_TYPE;
    QString SYS_TYPE;
    QString SEND_TIME;  //YYYY MM DD HH MM SS MSE 17자리
    QString IC_CODE;    //영업소번호 4자리
    QString LANE_NO;    //차로번호 2자리
    QString BD_NAME;    //보드명
    QString MSG_TYPE;   //메시지종류
    QString MAKER_NAME; //제조사영문  진우(JWIN)
    QString INTERFACE_VERSION;  //인터페이스버전,제조사마다 다를 수 있음 3자리
    QString FW_H_VERSION;

    //근무개시 데이터
    QString WORK_START_DATETIME;
    QString LANE_TYPE;
    QString LANE_SYS_TYPE;

    //차로상태 데이터
    QString LANE_KIND;
    QString MANUFACTURER;
    QString MANU_YEAR;
    QString FW_VERSION;
    QString FW_STATE;
    QString SYS_FORM_TYPE;
    QString CAMERA_CNT;
    QString CONTROLLERCONNECTION_STATE;
    QString FTPCONNECTION_STATE;
    //QString CAMERA_CNT  : 카메라개수
    //CAM_INFO  - 카메라정보
    QList<clsCameraSTATUS> cameraST;

    //보수데이터
    QString DEVICE_NAME;
    QString DEVICE_INDEX;
    QString REPAIR_EVENT;

    //펌웨어정보조회응답
    QString FW_DIV;
    QString FW_VER;

};


class VehcileIndex
{
public:
    VehcileIndex(int ch=0, int i=0)
    {
        channel = ch;
        index = i;
    }
    VehcileIndex( VehcileIndex *data)
    {
        channel = data->channel;
        index = data->index;
    }

    VehcileIndex &operator= (const VehcileIndex &other)
    {
        channel = other.channel;
        index = other.index;

        return *this;
    }

    int channel;
    int index;
};

class CameraSystem
{
public:
    CameraSystem()
    {
        TGIndex = 1; // 1 ~ FFFFH(65535)
        old_becs =0;
        TGLane = 1;
        MSindex = 1; // 1 ~ 9999
        ccu_viorepTimout = 100;
        /*        Recognition  value            */
        recog_status;
        recog_plateroi_sx = 0;
        recog_plateroi_sy = 0;
        recog_plateroi_width = 1920;
        recog_plateroi_height = 1080;
        recog_brroi_sx = 0;
        recog_brroi_sy = 0;
        recog_brroi_width = 1920;
        recog_brroi_height = 1080;
        recog_vehicleposition=0;
        recog_recognition_mode = 2;
        //recog_recogseq[8] = new int[8]{4, 3, 5, 2, 6, 1, 8, 7};
        recog_recogrepeattime = 4;
        recog_rawsavetype = 0;
        recog_rawsavecount = 0;

        lic_port = "ttyEX4";
        ccu_port = "ttyEX1";
    }

    /* Camera  */
   int cam_status;
   int cam_image_width;
   int cam_image_height;
   QString cam_ipaddress;
   uint cam_serialNumber;

   int colorflag;

   unsigned int cam_brightness;
   bool cam_brightness_auto;
   unsigned int cam_autoexposure;
   unsigned int cam_shutter;
   bool cam_shutter_auto;

   unsigned int cam_gain;
   bool cam_gain_auto;
   unsigned int cam_sharpness;
   unsigned int cam_hue;
   unsigned int cam_saturation;
   unsigned int cam_gamma;
   unsigned int cam_whitebalance_red;
   unsigned int cam_whitebalance_blue;
   bool cam_whitebalance_auto;
   bool cam_whitebalance_onoff;
   unsigned int cam_strobepolarity;

   /*             LIC        */
   int lic_status;
   QString lic_port;
   /*           CCU          */
   int ccu_status;
   QString ccu_port;
   int ccu_viorepTimout;

   int TGIndex; // 1 ~ FFFFH(65535)
   quint8 old_becs;
   uint TGLane;
   int MSindex; //1 ~9999

   /*        Recognition  value            */
   int recog_status;
   int recog_plateroi_sx;
   int recog_plateroi_sy;
   int recog_plateroi_width;
   int recog_plateroi_height;
   int recog_brroi_sx;
   int recog_brroi_sy;
   int recog_brroi_width;
   int recog_brroi_height;
   int recog_vehicleposition;
   int recog_recognition_mode;
   int recog_recogseq[8]={4, 3, 5, 2, 6, 1, 8, 7};
   int recog_recogrepeattime;
   int recog_rawsavetype;
   int recog_rawsavecount;

};



class  AutoIrisInfo
{
public:
     AutoIrisInfo()
     {
         /*        Autoiris             */
          autoiris_use = true;
          //basic
          autoiris_standardbr = 60;
          autoiris_brrange = 5;
          autoiris_brinterval = 1000;
          autoiris_bravgcount = 5;
          autoiris_maxshutter = 1000.0;
          autoiris_minshutter = 50.0;
          autoiris_shutterstep = 10.0;
          autoiris_maxgain = 16.0;
          autoiris_mingain = 0.0;
          autoiris_gainstep = 1.0;
          autoiris_debug = false;
          //reflection/back
          autoiris_reflection = true;
          autoiris_backlight = true;
          autoiris_maxbr = 200;
          autoiris_minbr = 30;
          autoiris_brstep = 10;
          autoiris_maxplatebr = 220;
          autoiris_minplatebr = 30;
          autoiris_brchangecount = 10;
          autoiris_reflectbackmode = 3;
          //no Recog
          autoiris_norecog = true;
          autoiris_norecogmaxbr = 200;
          autoiris_norecogminbr = 30;

          autoiris_lastshutter = 1000;
     }

     bool autoiris_use;
     //basic
     int autoiris_standardbr;
     int autoiris_brrange;
     int autoiris_brinterval;
     int autoiris_bravgcount;
     float autoiris_maxshutter;
     float autoiris_minshutter;
     float autoiris_shutterstep;
     float autoiris_maxgain;
     float autoiris_mingain;
     float autoiris_gainstep;
     bool autoiris_debug;
     //reflection/back
    bool autoiris_reflection;
    bool autoiris_backlight;
    int autoiris_maxbr;
    int autoiris_minbr;
    int autoiris_brstep;
    int autoiris_maxplatebr;
    int autoiris_minplatebr;
    int autoiris_brchangecount;
    int autoiris_reflectbackmode;
     //no Recog
     bool autoiris_norecog;
     int autoiris_norecogmaxbr;
     int autoiris_norecogminbr;
     //last shutter/gain
     float autoiris_lastshutter;
     float autoiris_lastgain;



};

class  AutoLightInfo
{
public:
    AutoLightInfo()
    {
        /*        Light             */
        light_24h = false;
        light_shutter = 1000.0;
        //int light_gain = 10;
        light_ongain = 16.0;
        light_offgain = 0.0;
    }
    /*        Light             */
     bool light_24h;
     float light_shutter;
     //sdw 2017/01/10
     //static int light_gain;
     float light_ongain;
     float light_offgain;
};

class CenterInfo
{
public:
    CenterInfo()
    {
        tcpport = 1944;
        connectioninterval = 1800;
        protocol_type = 0;
        ftpport = 21;
        fileNameSelect = 0;
    }
    QString centername;
    QString ip;
    int tcpport;
    int connectioninterval;
    enum Protocoltype
    {
        Normal = 0x00,      //TCP, FTP 모두 사용
        Remote = 0x01,      //TCP만 사용함. 프로토콜 다름
        FTP_Only = 0x02     //FTP만 사용함.
    };
    int protocol_type;

    int ftpport;
    QString userID;
    QString password;
    QString ftpPath;
    enum FileNameType
    {
        H_Char = 0x00,
        X_Char = 0x01
    };
    int fileNameSelect; //0=H , 1-X

    bool status;
    QLabel *plblstatus;
};

class VehicleData
{
public:
    VehicleData()
    {

    }

    // index , car count;
    quint16 seq;
    // tg num : XXXX to TG_XXXX=1
    int car_num;
    int ccu_seq;
    // car raw image. 1,2,3
    //QList<QImage *> img;
    unsigned char *img;
    unsigned int imglen;
//        // db & socket
    unsigned char *saveimg;
    unsigned int saveimglen;
    unsigned char *saveplateimg;
    unsigned int saveplateimglen;
//        QByteArray *saveimg;
//        QByteArray *saveplateimg;
    // car entrytime
    unsigned char car_entrytime[256];
    // camera color/black;
    bool img_color;
    //  화물, 승용차, 승용차
    int car_type;
    // 경차, 중형, 대형
    int size_type;
    //bool  차단 기종 류
    // cds
    int cds;
    // br
    int br;
    // 차 량 진 입방 향
    int direction;
    // 조 명 유/ 무
    bool light;
    // 속 도
    unsigned char speed[256];
    // 챠 량길 이
    int length;
    // 인 식번 호
    unsigned char recognum[256];
    // 인식 여부
    int recogresult;
    // 인 식시 간
    double recogtime;
    // 시프 트횟 수
    int recogshift;
    // raw image
    unsigned char *rawImage;
    unsigned int rawImagelen;
//        Image *rawImage;
    // 번호 판이미 지
    unsigned char *plateImage;
    unsigned int plateImagelen;
    //QImage *plateImage;
    // 번호 판 위 치
    int plate_x;
    // 번호 판위 치
    int plate_y;
    // 번호판 가로
    int plate_width;
    // 번호판 세로
    int plate_height;
    // ccu  - VehicleNotification
    bool bVehicleNotification;
    // 차량 처리 단계
    int vehicleproc;
};

class ConfirmData
{
public:
    ConfirmData()
    {
        seq = 0;
        imgNumber = 0;
        fFTPDatalen = 0;
        rFTPDatalen = 0;
        confirmValue = 0;
        fcardata = NULL;
        rcardata = NULL;

    }

    void init()
    {
        seq = 0;
        imgNumber = 0;
        fFTPDatalen = 0;
        rFTPDatalen = 0;
        confirmValue = 0;

        if(fcardata != NULL)
        {
            delete fcardata;
            fcardata = NULL;
        }
        if(rcardata != NULL)
        {
            delete rcardata;
            rcardata = NULL;
        }
    }


    quint8 seq;
    quint16 imgNumber;

    unsigned char *fFTPData;
    int fFTPDatalen;
    unsigned char frontNum[256];
    unsigned char frontFileName1[128];
    unsigned char frontFileName2[128];
    clsHipassCarData *fcardata;

    unsigned char *rFTPData;
    int rFTPDatalen;
    unsigned char rearNum[256];
    unsigned char rearFileName1[128];
    unsigned char rearFileName2[128];
    clsHipassCarData *rcardata;

    quint8 confirmValue;
};


class DataBaseInfo
{
public:
    DataBaseInfo()
    {
         db_status = 0;
         db_ip = "127.0.0.1";
         db_port = 3306;
         db_name = "lprdb";
         db_user = "root";
         db_password = "ubuntu";
         db_connectionTimeout = 5;

         db_commandTimeout = 5;
         db_storageDuration = 10;
    }
    int db_status;
    QString db_ip;
    int db_port;
    QString db_name;
    QString db_user;
    QString db_password;
    int db_connectionTimeout;

    int db_commandTimeout;
    int db_storageDuration;
};

class CurrentValue
{
public:
    CurrentValue()
    {
        cur_cds = 0;
        cur_br = 60;
        cur_shutter = 0.0;
        cur_gain = 0.0;
        cur_speed = 0;
        cur_temp = 0;
        cur_light = false;

        cur_fps = 0;

        cur_zoom = 0;
        cur_focus = 0;
        cur_iris = 0;
    }
    int cur_cds;
    int cur_br;
    float cur_shutter;
    float cur_gain;
    int cur_speed;
    int cur_temp;
    bool cur_light;

    int cur_zoom;
    int cur_focus;
    int cur_iris;

    int cur_fps;

};

class LocalDeviceStatus
{
public:
    LocalDeviceStatus()
    {
        //cds = 0;
        //temperature = 25;  //온도
        Humidity = 50; //습도
        fan = 0;
        heater = 0;
        door = 0;
        light = 0;

        booting = 0;
        camera = 0;
        server = 0;
        ccu = 0;
    }

    //quint16 cds;
    //quint8 temperature;  //온도
    quint8 Humidity; //습도
    quint8 fan;
    quint8 heater;
    quint8 door;
    quint8 light;

    quint8 booting;
    quint8 camera;
    quint8 server;
    quint8 ccu;

};

class clsFTPDATA
{
public:
    clsFTPDATA()
    {
        processNumber = 0;
        fileName = "";

    }
    ~clsFTPDATA()
    {


    }

    clsFTPDATA(quint32 pnumber, QString filName, QByteArray fdata,int rresult)
    {
        processNumber = pnumber;
        fileName = QString(filName.constData(),filName.size());
        ftpdata = QByteArray(fdata.constData(), fdata.size());
        recogresult = rresult;
    }
    QByteArray ftpdata;
    QString fileName;
    quint32 processNumber;
    int recogresult;

};

#endif // DATACLASS_H
